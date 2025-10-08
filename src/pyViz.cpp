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

PyVisualizer::PyVisualizer(int rows, int cols, int numVertices, const std::vector<int>& offsets,
    const std::vector<int>& neighbors, int plotTypes)
    : threadControls(std::make_shared<util::ThreadControls<VizUpdate>>())
    , gridRows(rows)
    , gridCols(cols)
    , numVertices(numVertices)
    , plotTypes(plotTypes)
    , offsets(offsets)
    , neighbors(neighbors)
{

    if (!initializePython()) {
        std::cerr << "Failed to initialize Python visualization" << std::endl;
        initialized = false;
        return;
    }

    if (!this->createVizInstance(plotTypes)) {
        std::cerr << "Failed to create visualization instance" << std::endl;
        shutdown();
        initialized = false;
        return;
    }

    if (plotTypes & util::GRID_MASK) {
        if (!this->initGrid(rows, cols, numVertices)) {
            std::cerr << "Failed to initialize grid visualization" << std::endl;
            shutdown();
            initialized = false;
            return;
        }
    }

    if (plotTypes & util::GRAPH_MASK) {
        if (!this->initGraph(offsets, neighbors)) {
            std::cerr << "Failed to initialize graph visualization" << std::endl;
            shutdown();
            initialized = false;
            return;
        }
    }

    if (plotTypes & util::STATS_MASK) {
        if (!this->initStats()) {
            std::cerr << "Failed to initialize stats visualization" << std::endl;
            shutdown();
            initialized = false;
            return;
        }
    }

    // CRITICAL: Release main thread's GIL so other threads can acquire it
    // Save the current thread state and release GIL
    this->mainThreadState = PyEval_SaveThread();
    std::cout << "Main thread released GIL, saved thread state" << std::endl;

    std::cout << "Python visualizer initialized (" << rows << "x" << cols << " grid) with "
              << numVertices << " vertices" << std::endl;

    initialized = true;
}

static PyObject* cStyleCallbackWrapper(PyObject* self, PyObject* /* args */)
{
    static const char* capsuleName = "PyVisualizerPtr";
    auto* that = reinterpret_cast<PyVisualizer*>(PyCapsule_GetPointer(self, capsuleName));
    if (!that) {
        if (PyErr_Occurred())
            PyErr_Print();
        Py_RETURN_NONE;
    }
    that->onWindowCloseCallback();
    Py_RETURN_NONE;
}

bool PyVisualizer::createVizInstance(int plotTypes)
{
    // Acquire GIL for Python operations
    PyGILState_STATE gstate = PyGILState_Ensure();

    if (pVizInstance) {
        std::cerr << "Visualizer instance already exists" << std::endl;
        PyGILState_Release(gstate);
        return false;
    }

    PyObject* pArgs = PyTuple_New(2);
    PyTuple_SetItem(pArgs, 0, PyLong_FromLong(plotTypes));
    PyTuple_SetItem(pArgs, 1, this->createOnCloseCallback());

    this->pVizInstance = PyObject_CallObject(pVizClass, pArgs);
    if (!this->pVizInstance) {
        PyErr_Print();
        std::cerr << "Failed to create Viz instance" << std::endl;
        PyGILState_Release(gstate);
        return false;
    }

    PyGILState_Release(gstate);
    return true;
}

bool PyVisualizer::initGrid(int rows, int cols, int numVertices)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pResult
        = PyObject_CallMethod(pVizInstance, "init_grid", "iii", rows, cols, numVertices);
    if (!pResult) {
        std::cerr << "Failed to call init_grid method" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return false;
    }

    Py_DECREF(pResult);
    PyGILState_Release(gstate);
    return true;
}

bool PyVisualizer::initGraph(const std::vector<int>& offsets, const std::vector<int>& neighbors)
{

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pEdgesList = PyList_New(neighbors.size());
    if (!pEdgesList) {
        std::cerr << "Failed to create Python list for edges" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return false;
    }

    for (size_t i = 0; i < neighbors.size(); i++) {
        for (int j = offsets[i]; j < offsets[i + 1]; j++) {
            PyObject* pEdge = PyTuple_New(2);
            if (!pEdge) {
                Py_DECREF(pEdgesList);
                std::cerr << "Failed to create edge tuple" << std::endl;
                PyErr_Print();
                PyGILState_Release(gstate);
                return false;
            }
            PyTuple_SetItem(pEdge, 0, PyLong_FromLong(i));
            PyTuple_SetItem(pEdge, 1, PyLong_FromLong(neighbors[j]));
            PyList_SetItem(pEdgesList, j, pEdge);
        }
    }

    PyObject* pResult = PyObject_CallMethod(pVizInstance, "init_graph", "O", pEdgesList);
    if (!pResult) {
        std::cerr << "Failed to call init_graph method" << std::endl;
        PyErr_Print();
        Py_DECREF(pEdgesList);
        PyGILState_Release(gstate);
        return false;
    }

    Py_DECREF(pResult);
    PyGILState_Release(gstate);
    return true;
}

bool PyVisualizer::initStats()
{
    std::cerr << "Stats visualization not yet implemented" << std::endl;
    return false;
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
    }

    // Acquire GIL for this thread
    PyGILState_STATE gstate = PyGILState_Ensure();

    // Add current directory to Python path
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./scripts')");

    // Import our module for visualization
    pVizModule = PyImport_ImportModule("Viz");
    if (!pVizModule) {
        PyErr_Print();
        std::cerr << "Failed to import the visualization module" << std::endl;
        return false;
    }

    // Get Viz class
    pVizClass = PyObject_GetAttrString(pVizModule, "Viz");
    if (!pVizClass || !PyCallable_Check(pVizClass)) {
        PyErr_Print();
        std::cerr << "Failed to get Viz class" << std::endl;
        return false;
    }

    pVizUpdateClass = PyObject_GetAttrString(pVizModule, "VizUpdate");
    if (!pVizUpdateClass || !PyCallable_Check(pVizUpdateClass)) {
        PyErr_Print();
        std::cerr << "Failed to get VizUpdate class" << std::endl;
        return false;
    }

    // Release GIL
    PyGILState_Release(gstate);
    return true;
}

bool PyVisualizer::isActive()
{
    if (!initialized)
        return false;

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pResult = PyObject_CallMethod(pVizInstance, "viz_active", nullptr);
    if (!pResult) {
        std::cerr << "Failed to call viz_active method" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return false;
    }

    int isActive = PyObject_IsTrue(pResult);
    Py_DECREF(pResult);
    PyGILState_Release(gstate);

    return isActive;
}

void PyVisualizer::displayFigures()
{
    if (!initialized) {
        std::cerr << "Visualizer not initialized, cannot display figures" << std::endl;
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pResult = PyObject_CallMethod(pVizInstance, "show", nullptr);
    if (!pResult) {
        std::cerr << "Failed to call show method" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    Py_DECREF(pResult);
    PyGILState_Release(gstate);
}

void PyVisualizer::updateViz(const std::vector<util::Position>& positions, double score)
{
    if (!initialized) {
        std::cerr << "Visualizer not initialized, cannot update" << std::endl;
        return;
    }

    if ((plotTypes & (util::GRID_MASK | util::STATS_MASK)) == 0) {
        std::cerr << "Update called but no grid or stats visualization enabled" << std::endl;
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pPosList = PyList_New(positions.size());
    if (!pPosList) {
        std::cerr << "Failed to create Python list for positions" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    for (size_t i = 0; i < positions.size(); i++) {
        PyObject* pPosTuple = PyTuple_New(2);
        if (!pPosTuple) {
            Py_DECREF(pPosList);
            std::cerr << "Failed to create position tuple" << std::endl;
            PyErr_Print();
            PyGILState_Release(gstate);
            return;
        }

        PyTuple_SetItem(pPosTuple, 0, PyLong_FromLong(positions[i].row));
        PyTuple_SetItem(pPosTuple, 1, PyLong_FromLong(positions[i].col));
        PyList_SetItem(pPosList, i, pPosTuple);
    }

    PyObject* pUpdateArgs = PyTuple_New(2);
    if (!pUpdateArgs) {
        std::cerr << "Failed to create update arguments tuple" << std::endl;
        PyErr_Print();
        Py_DECREF(pPosList);
        PyGILState_Release(gstate);
        return;
    }

    PyTuple_SetItem(pUpdateArgs, 0, pPosList);
    PyTuple_SetItem(pUpdateArgs, 1, PyFloat_FromDouble(score));

    PyObject* pUpdateInstance = PyObject_CallObject(pVizUpdateClass, pUpdateArgs);
    if (!pUpdateInstance) {
        std::cerr << "Failed to create VizUpdate instance" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    PyObject* pResult = PyObject_CallMethod(pVizInstance, "update", "O", pUpdateInstance);
    if (!pResult) {
        std::cerr << "Failed to call update method" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    Py_DECREF(pResult);
    PyGILState_Release(gstate);
}

void PyVisualizer::keepAlive(double pauseTime)
{
    if (!initialized) {
        std::cerr << "Visualizer not initialized, cannot keep alive" << std::endl;
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pResult
        = PyObject_CallMethod(pVizInstance, "keep_alive", "O", PyFloat_FromDouble(pauseTime));
    if (!pResult) {
        std::cerr << "Failed to call keep_alive method" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    Py_DECREF(pResult);
    PyGILState_Release(gstate);
}

void PyVisualizer::saveFigures(const std::string& gridAnimationFilename,
    const std::string& gridStaticFilename, const std::string& graphFilename,
    const std::string& statsFilename)
{
    if (!initialized) {
        std::cerr << "Visualizer not initialized, cannot save figures" << std::endl;
        return;
    }

    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* pResult
        = PyObject_CallMethod(pVizInstance, "save_figs", "ssss", gridAnimationFilename.c_str(),
            gridStaticFilename.c_str(), graphFilename.c_str(), statsFilename.c_str());
    if (!pResult) {
        std::cerr << "Failed to call save_figs method" << std::endl;
        PyErr_Print();
        PyGILState_Release(gstate);
        return;
    }

    Py_DECREF(pResult);
    PyGILState_Release(gstate);
}

void PyVisualizer::onWindowCloseCallback()
{
    {
        std::unique_lock<std::mutex> lock(this->threadControls->queueMutex);
        this->threadControls->shouldStop.store(true);
    }
    this->threadControls->queueCondition.notify_all();
}

PyObject* PyVisualizer::createOnCloseCallback()
{
    static const char* capsuleName = "PyVisualizerPtr";

    // Create a capsule to hold the callback function pointer
    PyObject* capsule = PyCapsule_New(static_cast<void*>(this), capsuleName, nullptr);
    if (!capsule) {
        std::cerr << "Failed to create close_callback capsule" << std::endl;
        return nullptr;
    }

    static PyMethodDef methodDef
        = { "close_callback", reinterpret_cast<PyCFunction>(cStyleCallbackWrapper), METH_NOARGS,
              "Notify C++ that the window was closed" };

    PyObject* func = PyCFunction_NewEx(&methodDef, capsule, nullptr);
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

    if (pVizInstance) {
        Py_DECREF(pVizInstance);
        pVizInstance = nullptr;
    }
    if (pVizClass) {
        Py_DECREF(pVizClass);
        pVizClass = nullptr;
    }
    if (pVizModule) {
        Py_DECREF(pVizModule);
        pVizModule = nullptr;
    }
}

void PyVisualizer::shutdown()
{
    if (!initialized)
        return;

    cleanupPython();
    initialized = false;
}

// Public C++ API

void visualizationLoop(PyVisualizer& viz)
{
    // // This thread needs to be registered with Python
    // PyGILState_STATE gstate = PyGILState_Ensure();
    // PyGILState_Release(gstate); // Release immediately, acquire per operation

    using namespace std::chrono_literals;
    const auto tick = 33ms; // Approx 30 FPS

    while (!viz.threadControls->shouldStop.load()) {

        VizUpdate update;

        {
            std::unique_lock<std::mutex> lock(viz.threadControls->queueMutex);

            // Wait for updates or shutdown
            viz.threadControls->queueCondition.wait_for(lock, tick, [&] {
                return !viz.threadControls->messageQueue.empty()
                    || viz.threadControls->shouldStop.load();
            });

            if (viz.threadControls->shouldStop.load() && viz.threadControls->messageQueue.empty())
                break;

            // Process all queued updates
            if (!viz.threadControls->messageQueue.empty()) {
                update = std::move(viz.threadControls->messageQueue.front());
                viz.threadControls->messageQueue.pop();
            }
        }

        // Keep the UI alive
        viz.keepAlive(0.01);

        // Process updates outside the lock
        viz.updateViz(update.positions, update.score);
    }

    {
        std::unique_lock<std::mutex> lock(viz.threadControls->queueMutex);
        std::queue<VizUpdate> empty;
        std::swap(viz.threadControls->messageQueue, empty);
    }
}

};
#endif // HAVE_PYTHON
