
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
    std::vector<std::pair<int, int>> edges;
    int iteration = 0;
    double score = 0.0;
    int gridRows = 0;
    int gridCols = 0;

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

    PyObject* pModule = nullptr;
    PyObject* pGridClass = nullptr;
    PyObject* pGridInstance = nullptr;
    PyObject* pGraphFunc = nullptr;

    int gridRows = 0;
    int gridCols = 0;
    bool initialized = false;

public:
    PyVisualizer() = default;
    ~PyVisualizer() { shutdown(); }

    bool initialize(int rows, int cols);
    void displayGraph(const std::vector<std::pair<int, int>>& edges);
    void updateGrid(const std::vector<util::Position>& positions, int iteration, double score);
    void shutdown();

private:
    void visualizationLoop();
    bool initializePython();
    void cleanupPython();
    void processGraphDisplay(const VizUpdate& update);
    void processGridUpdate(const VizUpdate& update);
};

// Global instance
static PyVisualizer g_visualizer;

bool PyVisualizer::initialize(int rows, int cols)
{
    if (initialized)
        return true;

    gridRows = rows;
    gridCols = cols;

    if (!initializePython()) {
        std::cerr << "Failed to initialize Python visualization" << std::endl;
        return false;
    }

    // Start visualization thread
    vizThread = std::thread(&PyVisualizer::visualizationLoop, this);
    initialized = true;

    std::cout << "Python visualizer initialized (" << rows << "x" << cols << " grid)" << std::endl;
    return true;
}

bool PyVisualizer::initializePython()
{
    // Initialize Python interpreter
    if (!Py_IsInitialized()) {
        Py_Initialize();
        if (!Py_IsInitialized()) {
            std::cerr << "Failed to initialize Python" << std::endl;
            return false;
        }
    }

    // Add current directory to Python path
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./scripts')");

    // Import our visualization module
    pModule = PyImport_ImportModule("viz");
    if (!pModule) {
        PyErr_Print();
        std::cerr << "Failed to import viz module" << std::endl;
        return false;
    }

    // Get AnimatedGrid class
    pGridClass = PyObject_GetAttrString(pModule, "AnimatedGrid");
    if (!pGridClass || !PyCallable_Check(pGridClass)) {
        PyErr_Print();
        std::cerr << "Failed to get AnimatedGrid class" << std::endl;
        return false;
    }

    // Get test_graph function
    pGraphFunc = PyObject_GetAttrString(pModule, "test_graph");
    if (!pGraphFunc || !PyCallable_Check(pGraphFunc)) {
        PyErr_Print();
        std::cerr << "Failed to get test_graph function" << std::endl;
        return false;
    }

    return true;
}

void PyVisualizer::visualizationLoop()
{
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

    cleanupPython();
}

void PyVisualizer::processGraphDisplay(const VizUpdate& update)
{
    // For now, just call the test_graph function
    // TODO: Create a custom graph display function that takes edges as parameter
    if (pGraphFunc) {
        PyObject* pResult = PyObject_CallObject(pGraphFunc, nullptr);
        if (!pResult) {
            PyErr_Print();
        } else {
            Py_DECREF(pResult);
        }
    }
}

void PyVisualizer::processGridUpdate(const VizUpdate& update)
{
    if (!pGridInstance) {
        // Create grid instance on first update
        PyObject* pArgs = PyTuple_New(3);
        PyTuple_SetItem(pArgs, 0, PyLong_FromLong(update.gridRows));
        PyTuple_SetItem(pArgs, 1, PyLong_FromLong(update.gridCols));
        PyTuple_SetItem(pArgs, 2, PyUnicode_FromString("C++ Annealing Visualization"));

        pGridInstance = PyObject_CallObject(pGridClass, pArgs);
        Py_DECREF(pArgs);

        if (!pGridInstance) {
            PyErr_Print();
            return;
        }

        // Enable live update mode
        PyObject* pLiveMethod = PyObject_GetAttrString(pGridInstance, "live_update_mode");
        if (pLiveMethod && PyCallable_Check(pLiveMethod)) {
            PyObject* pResult = PyObject_CallObject(pLiveMethod, nullptr);
            if (pResult)
                Py_DECREF(pResult);
            Py_DECREF(pLiveMethod);
        }
    }

    // Update grid positions
    PyObject* pUpdateMethod = PyObject_GetAttrString(pGridInstance, "update_positions");
    if (!pUpdateMethod || !PyCallable_Check(pUpdateMethod)) {
        std::cerr << "Failed to get update_positions method" << std::endl;
        return;
    }

    // Convert positions to Python list
    PyObject* pPositionsList = PyList_New(update.positions.size());
    for (size_t i = 0; i < update.positions.size(); ++i) {
        PyObject* pTuple = PyTuple_New(2);
        PyTuple_SetItem(pTuple, 0, PyLong_FromLong(update.positions[i].row));
        PyTuple_SetItem(pTuple, 1, PyLong_FromLong(update.positions[i].col));
        PyList_SetItem(pPositionsList, i, pTuple);
    }

    // Create arguments for update_positions
    PyObject* pArgs = PyTuple_New(4);
    PyTuple_SetItem(pArgs, 0, pPositionsList);
    PyTuple_SetItem(pArgs, 1, Py_None);
    Py_INCREF(Py_None); // node_indices
    PyTuple_SetItem(pArgs, 2, PyLong_FromLong(update.iteration));
    PyTuple_SetItem(pArgs, 3, PyFloat_FromDouble(update.score));

    // Call update_positions
    PyObject* pResult = PyObject_CallObject(pUpdateMethod, pArgs);
    if (!pResult) {
        PyErr_Print();
    } else {
        Py_DECREF(pResult);
    }

    Py_DECREF(pArgs);
    Py_DECREF(pUpdateMethod);
}

void PyVisualizer::displayGraph(const std::vector<std::pair<int, int>>& edges)
{
    if (!initialized)
        return;

    VizUpdate update(VizUpdate::GRAPH_DISPLAY);
    update.edges = edges;

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
    update.gridRows = gridRows;
    update.gridCols = gridCols;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        updateQueue.push(update);
    }
    queueCondition.notify_one();
}

void PyVisualizer::cleanupPython()
{
    if (pGridInstance) {
        Py_DECREF(pGridInstance);
        pGridInstance = nullptr;
    }
    if (pGridClass) {
        Py_DECREF(pGridClass);
        pGridClass = nullptr;
    }
    if (pGraphFunc) {
        Py_DECREF(pGraphFunc);
        pGraphFunc = nullptr;
    }
    if (pModule) {
        Py_DECREF(pModule);
        pModule = nullptr;
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
    std::cout << "Python visualizer shut down" << std::endl;
}

// Public C++ API
bool initializeVisualizer(int gridRows, int gridCols)
{
    return g_visualizer.initialize(gridRows, gridCols);
}

void displayGraph(const std::vector<std::pair<int, int>>& edges)
{
    g_visualizer.displayGraph(edges);
}

void updateVisualization(const std::vector<util::Position>& positions, int iteration, double score)
{
    // Only update every 100 iterations to avoid overwhelming the display
    static int lastUpdate = -100;
    if (iteration - lastUpdate >= 100 || iteration == 0) {
        g_visualizer.updateGrid(positions, iteration, score);
        lastUpdate = iteration;
    }
}

void shutdownVisualizer() { g_visualizer.shutdown(); }

} // namespace viz