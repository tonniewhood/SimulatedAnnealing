/**
 * @file pyViz.cpp
 * @brief C++ interface to Python visualization for graph annealing
 *
 * This file implements a C++ interface to a Python-based visualization system
 * for visualizing the graph annealing process. It uses the Python C API to
 * initialize the Python interpreter, load visualization modules, and update the
 * visualization in a separate thread.
 *
 * I should state, that this was largely created by Github Copilot. I'm not super familiar
 * with the embedded Python C API, so I let Copilot generate a lot of the boilerplate
 * code for me. I then modified it to fit my needs. I don't want to take full credit as it's
 * definitely not solely my work.
 *
 * One of the big issues I faced was fighing with the Global Interpreter Lock (GIL). Copilot did
 * a lot of the heavy lifting in regard to where I need to acquire and release the GIL and maybe
 * more importantly how.
 */

#ifdef HAVE_PYTHON

#include <Python.h>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Graph.hpp"
#include "pyViz.hpp"
#include "util.hpp"

namespace viz {

// Thread-safe data structure for visualization updates
struct VizUpdate {
    enum Type { GRAPH_DISPLAY, GRID_UPDATE, SHUTDOWN };

    Type type;
    std::vector<util::Position> positions;
    int iteration = 0;
    double score = 0.0;

    VizUpdate(Type t)
        : type(t)
    {
    }
};

class PyVisualizer {
private:
    std::thread vizThread;
    std::queue<VizUpdate> updateQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> shouldStop { false };

    PyObject* pGridModule = nullptr;
    PyObject* pGraphModule = nullptr;
    PyObject* pGridClass = nullptr;
    PyObject* pGraphClass = nullptr;
    PyObject* pGridInstance = nullptr;
    PyObject* pGraphInstance = nullptr;

    int gridRows = 0;
    int gridCols = 0;
    int numVertices = 0;
    bool initialized = false;
    bool isActiveFlag = false;
    int numWindowsActive = 0; // Don't need to keep it above 0, and the check to quit happens inside
                              // of a callback which will only be called after the caller is
                              // created, which will increment the count above 0

    std::vector<int> offsets = {};
    std::vector<int> neighbors = {};
    std::vector<util::Position> lastPositions = {};

    // Store main thread state for proper GIL management
    PyThreadState* mainThreadState = nullptr;

public:
    PyVisualizer() = default;
    ~PyVisualizer() { shutdown(); }

    bool initialize(int rows, int cols, int numVertices, const std::vector<int>& offsets,
        const std::vector<int>& neighbors);
    void displayGraph();
    void updateGrid(const std::vector<util::Position>& positions, int iteration, double score);
    void onWindowCloseCallback();
    bool isActive() const { return this->isActiveFlag; }
    void shutdown();

private:
    PyObject* createOnCloseCallback();
    void visualizationLoop();
    bool initializePython();
    void cleanupPython();
    void processGraphDisplay(const VizUpdate& update);
    void processGridUpdate(const VizUpdate& update);
};

// Global instances
static PyVisualizer g_visualizer;
static PyObject* cStyleCallbackWrapper(PyObject* self, PyObject* /* args */)
{
    auto* that = reinterpret_cast<PyVisualizer*>(PyCapsule_GetPointer(self, nullptr));
    if (that) {
        that->onWindowCloseCallback();
    }
    Py_RETURN_NONE;
}

bool PyVisualizer::initialize(int rows, int cols, int numVertices, const std::vector<int>& offsets,
    const std::vector<int>& neighbors)
{
    if (initialized)
        return true;

    this->gridRows = rows;
    this->gridCols = cols;
    this->numVertices = numVertices;
    this->offsets = offsets;
    this->neighbors = neighbors;

    if (!initializePython()) {
        std::cerr << "Failed to initialize Python visualization" << std::endl;
        return false;
    }

    // CRITICAL: Release main thread's GIL so other threads can acquire it
    // Save the current thread state and release GIL
    this->mainThreadState = PyEval_SaveThread();
    std::cout << "Main thread released GIL, saved thread state" << std::endl;

    // Start visualization thread
    vizThread = std::thread(&PyVisualizer::visualizationLoop, this);
    initialized = true;

    std::cout << "Python visualizer initialized (" << rows << "x" << cols << " grid) with "
              << numVertices << " vertices" << std::endl;
    return true;
}

bool PyVisualizer::initializePython()
{
    // Initialize Python interpreter with threading support
    if (!Py_IsInitialized()) {
        Py_Initialize();
        if (!Py_IsInitialized()) {
            std::cerr << "Failed to initialize Python" << std::endl;
            return false;
        }

        // Initialize threading support (deprecated in Python 3.7+ but still works)
        if (!PyEval_ThreadsInitialized()) {
            PyEval_InitThreads();
        }
    }

    // Acquire GIL for this thread
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Add current directory to Python path
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./scripts')");

    // Import our module for grid visualization
    pGridModule = PyImport_ImportModule("AnimatedGrid");
    if (!pGridModule) {
        PyErr_Print();
        std::cerr << "Failed to import grid visualization module" << std::endl;
        return false;
    }

    // Import our graph visualization module
    pGraphModule = PyImport_ImportModule("BasicDigraph");
    if (!pGraphModule) {
        PyErr_Print();
        std::cerr << "Failed to import graph visualization module" << std::endl;
        return false;
    }

    // Get AnimatedGrid class
    pGridClass = PyObject_GetAttrString(pGridModule, "AnimatedGrid");
    if (!pGridClass || !PyCallable_Check(pGridClass)) {
        PyErr_Print();
        std::cerr << "Failed to get AnimatedGrid class" << std::endl;
        return false;
    }

    // Get BasicDigraph class
    pGraphClass = PyObject_GetAttrString(pGraphModule, "BasicDigraph");
    if (!pGraphClass || !PyCallable_Check(pGraphClass)) {
        PyErr_Print();
        std::cerr << "Failed to get BasicDigraph class" << std::endl;
        PyGILState_Release(gstate);
        return false;
    }

    this->isActiveFlag = true;

    // Release GIL
    PyGILState_Release(gstate);
    return true;
}

void PyVisualizer::visualizationLoop()
{
    // This thread needs to be registered with Python
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyGILState_Release(gstate); // Release immediately, acquire per operation

    while (!shouldStop) {
        std::unique_lock<std::mutex> lock(queueMutex);

        // Wait for updates or shutdown
        queueCondition.wait(lock, [this] { return !updateQueue.empty() || shouldStop; });

        if (shouldStop)
            break;

        // Process all queued updates
        while (!updateQueue.empty()) {

            VizUpdate update = updateQueue.front();
            updateQueue.pop();
            lock.unlock();

            switch (update.type) {
            case VizUpdate::GRAPH_DISPLAY:
                processGraphDisplay(update);
                break;
            case VizUpdate::GRID_UPDATE:
                processGridUpdate(update);
                break;
            case VizUpdate::SHUTDOWN:
                shouldStop = true;
                break;
            }

            lock.lock();
        }
    }

    this->isActiveFlag = false;
    cleanupPython();
}

void PyVisualizer::processGraphDisplay(const VizUpdate& update)
{
    // Acquire GIL for Python operations
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (!pGraphInstance) {
        // Create graph instance on first display (probably will also only be the only display)
        std::cout << "Creating graph with " << this->numVertices << " vertices for visualization"
                  << std::endl;

        // std::cout << "Total edges: " << totalEdges << std::endl;
        PyObject* pEdges = PyList_New(this->neighbors.size());
        if (!pEdges) {
            std::cerr << "Failed to create Python list for edges" << std::endl;
            PyGILState_Release(gstate);
            return;
        }

        int edgeIndex = 0;
        for (int i = 0; i < numVertices; i++) {
            for (int j = offsets[i]; j < offsets[i + 1]; j++) {
                PyObject* pEdge = PyTuple_New(2);
                if (!pEdge) {
                    std::cerr << "Failed to create edge tuple" << std::endl;
                    Py_DECREF(pEdges);
                    PyGILState_Release(gstate);
                    return;
                }

                PyTuple_SetItem(pEdge, 0, PyLong_FromLong(i));
                PyTuple_SetItem(pEdge, 1, PyLong_FromLong(neighbors[j]));
                PyList_SetItem(pEdges, edgeIndex++, pEdge);
            }
        }

        PyObject* pArgs = PyTuple_New(1);
        PyTuple_SetItem(pArgs, 0, pEdges);

        pGraphInstance = PyObject_CallObject(pGraphClass, pArgs);
        Py_DECREF(pArgs);

        if (!pGraphInstance) {
            PyErr_Print();
            PyGILState_Release(gstate);
            std::cerr << "Failed to create graph instance" << std::endl;
            return;
        }

        // Call display method
        PyObject* pResult = PyObject_CallMethod(pGraphInstance, "display", nullptr);
        if (!pResult) {
            PyErr_Print();
            PyGILState_Release(gstate);
            std::cerr << "Failed to call display method on graph instance" << std::endl;
            return;
        }
        Py_DECREF(pResult);

        // Increment active window count
        this->numWindowsActive++;
    }

    // Release GIL
    PyGILState_Release(gstate);
    // For now, we only display the graph once at the start
    // Future updates could modify the graph if needed
}

void PyVisualizer::processGridUpdate(const VizUpdate& update)
{
    // Acquire GIL for Python operations
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (!pGridInstance) {
        // Create grid instance on first update
        PyObject* pArgs = PyTuple_New(6);
        PyTuple_SetItem(pArgs, 0, PyLong_FromLong(this->gridRows));
        PyTuple_SetItem(pArgs, 1, PyLong_FromLong(this->gridCols));
        PyTuple_SetItem(pArgs, 2, PyUnicode_FromString("C++ Annealing Visualization"));
        PyTuple_SetItem(pArgs, 3, PyBool_FromLong(1 /* use_color_map */));
        PyTuple_SetItem(pArgs, 4, PyLong_FromLong(this->numVertices));

        PyObject* pCallback = createOnCloseCallback();
        if (!pCallback) {
            PyErr_Print();
            PyGILState_Release(gstate);
            Py_DECREF(pArgs);
            std::cerr << "Failed to create on_close_callback" << std::endl;
            return;
        }
        PyTuple_SetItem(pArgs, 5, pCallback);

        pGridInstance = PyObject_CallObject(pGridClass, pArgs);
        Py_DECREF(pArgs);

        if (!pGridInstance) {
            PyErr_Print();
            PyGILState_Release(gstate);
            std::cerr << "Failed to create grid instance" << std::endl;
            return;
        }

        PyObject* pResult = PyObject_CallMethod(pGridInstance, "live_update_mode", nullptr);
        if (!pResult) {
            PyErr_Print();
            PyGILState_Release(gstate);
            std::cerr << "Failed to set live update mode on grid instance" << std::endl;
            return;
        }
        Py_DECREF(pResult);

        // Increment active window count
        this->numWindowsActive++;
    }

    // Prepare positions as a Python list of tuples
    PyObject* pPosList = PyList_New(update.positions.size());
    if (!pPosList) {
        PyGILState_Release(gstate);
        std::cerr << "Failed to create Python list for positions" << std::endl;
        return;
    }

    for (size_t i = 0; i < update.positions.size(); i++) {
        PyObject* pPosTuple = PyTuple_New(2);
        if (!pPosTuple) {
            Py_DECREF(pPosList);
            PyGILState_Release(gstate);
            std::cerr << "Failed to create position tuple" << std::endl;
            return;
        }

        PyTuple_SetItem(pPosTuple, 0, PyLong_FromLong(update.positions[i].row));
        PyTuple_SetItem(pPosTuple, 1, PyLong_FromLong(update.positions[i].col));
        PyList_SetItem(pPosList, i, pPosTuple);
    }

    PyObject* pResult = PyObject_CallMethod(pGridInstance, "update_positions", "OOllO", pPosList,
        Py_None, PyLong_FromLong(update.iteration), PyLong_FromLong(update.score),
        PyBool_FromLong(1), nullptr);
    if (!pResult) {
        PyErr_Print();
        Py_DECREF(pPosList);
        PyGILState_Release(gstate);
        std::cerr << "Failed to call update_positions on grid instance" << std::endl;
        return;
    }
    Py_DECREF(pResult);
    Py_DECREF(pPosList);

    // Release GIL
    PyGILState_Release(gstate);
}

void PyVisualizer::displayGraph()
{
    if (!initialized)
        return;

    VizUpdate update(VizUpdate::GRAPH_DISPLAY);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        updateQueue.push(update);
    }
    queueCondition.notify_one();
}

void PyVisualizer::updateGrid(
    const std::vector<util::Position>& positions, int iteration, double score)
{
    if (!initialized)
        return;

    VizUpdate update(VizUpdate::GRID_UPDATE);
    update.positions = positions;
    update.iteration = iteration;
    update.score = score;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        updateQueue.push(update);
    }
    queueCondition.notify_one();
}

void PyVisualizer::onWindowCloseCallback()
{
    // This could be called from Python via a callback when the window is closed
    std::cout << "Visualization window closed by user" << std::endl;
    std::cout << "Windows remaining: " << this->numWindowsActive - 1 << std::endl;

    // If all windows are closed, we can signal shutdown
    if (--this->numWindowsActive <= 0) {
        this->shouldStop = true;
        this->isActiveFlag = false;
        queueCondition.notify_one();
    }
}

PyObject* PyVisualizer::createOnCloseCallback()
{
    // Create a capsule to hold the callback function pointer
    PyObject* capsule = PyCapsule_New(
        (void*)(&PyVisualizer::onWindowCloseCallback), "on_close_callback", nullptr);

    if (!capsule) {
        std::cerr << "Failed to create on_close_callback capsule" << std::endl;
        return nullptr;
    }

    PyMethodDef* methodDef = new PyMethodDef { "on_close_callback",
        reinterpret_cast<PyCFunction>(cStyleCallbackWrapper), METH_NOARGS,
        "Notify C++ that the window was closed" };

    PyObject* func = PyCFunction_New(methodDef, nullptr);
    if (!func) {
        PyErr_Print();
        std::cerr << "Failed to create on_close_callback function" << std::endl;
        Py_DECREF(capsule);
        return nullptr;
    }

    Py_DECREF(capsule);
    return func;
}

void PyVisualizer::cleanupPython()
{
    // Restore main thread state to cleanup Python objects
    if (mainThreadState) {
        PyEval_RestoreThread(mainThreadState);
        mainThreadState = nullptr;
    }

    if (pGridInstance) {
        Py_DECREF(pGridInstance);
        pGridInstance = nullptr;
    }
    if (pGridClass) {
        Py_DECREF(pGridClass);
        pGridClass = nullptr;
    }
    if (pGridModule) {
        Py_DECREF(pGridModule);
        pGridModule = nullptr;
    }
    if (pGraphInstance) {
        Py_DECREF(pGraphInstance);
        pGraphInstance = nullptr;
    }
    if (pGraphClass) {
        Py_DECREF(pGraphClass);
        pGraphClass = nullptr;
    }
    if (pGraphModule) {
        Py_DECREF(pGraphModule);
        pGraphModule = nullptr;
    }
}

void PyVisualizer::shutdown()
{
    if (!initialized)
        return;

    shouldStop = true;

    {
        VizUpdate update(VizUpdate::SHUTDOWN);
        std::lock_guard<std::mutex> lock(queueMutex);
        updateQueue.push(update);
    }
    queueCondition.notify_one();

    if (vizThread.joinable()) {
        vizThread.join();
    }

    initialized = false;
}

// Public C++ API
bool initializeVisualizer(int gridRows, int gridCols, int numVertices,
    const std::vector<int>& offsets, const std::vector<int>& neighbors)
{
    return g_visualizer.initialize(gridRows, gridCols, numVertices, offsets, neighbors);
}

void displayGraph() { g_visualizer.displayGraph(); }

bool isActive() { return g_visualizer.isActive(); }

void updateVisualization(const std::vector<util::Position>& positions, int iteration, double score)
{
    g_visualizer.updateGrid(positions, iteration, score);
}

void shutdownVisualizer() { g_visualizer.shutdown(); }

} // namespace viz

#else

#include <iostream>

#include "pyViz.hpp"

namespace viz {

bool initializeVisualizer(int, int)
{
    std::cerr << "Python visualization not available (HAVE_PYTHON not defined)" << std::endl;
    return false;
}

void displayGraph(const std::vector<int>&, const std::vector<int>&)
{
    std::cerr << "Python visualization not available (HAVE_PYTHON not defined)" << std::endl;
}

void updateVisualization(const std::vector<util::Position>&, int, double)
{
    std::cerr << "Python visualization not available (HAVE_PYTHON not defined)" << std::endl;
}

void shutdownVisualizer()
{
    std::cerr << "Python visualization not available (HAVE_PYTHON not defined)" << std::endl;
}
} // namespace viz

#endif // HAVE_PYTHON
