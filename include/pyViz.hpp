

#ifndef PYVIZ_HPP
#define PYVIZ_HPP

#ifdef HAVE_PYTHON

#include <Python.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "util.hpp"

namespace viz {

struct VizUpdate {

    std::vector<util::Position> positions;
    double score = 0.0;

    VizUpdate() = default;
};
using ThreadControlPtr = std::shared_ptr<util::ThreadControls<VizUpdate>>;

class PyVisualizer {

public:
    /**
     * @brief Initialize the Python visualization system
     * @param gridRows Number of rows in the grid
     * @param gridCols Number of columns in the grid
     * @param numVertices Number of vertices in the graph
     * @param offsets Adjacency list offsets for the graph
     * @param neighbors Adjacency list neighbors for the graph
     * @param initialPositions Initial positions of the vertices
     * @return true if initialization successful
     */
    PyVisualizer(int rows, int cols, int numVertices, const std::vector<int>& offsets,
        const std::vector<int>& neighbors, int plotTypes = 3);
    ~PyVisualizer() { shutdown(); }

    ThreadControlPtr threadControls;

    /**
     * @brief Check if the visualizer was initialized successfully
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }

    /**
     * @brief Check if the visualization windows are still active
     * @return true if active, false if shutdown
     */
    bool isActive();

    /**
     * @brief Display the figures. Dependant on flags at initialization
     */
    void displayFigures();

    /**
     * @brief Update the grid visualization with current positions
     * @param positions Current vertex positions
     * @param score Current annealing score
     * @note Only updates at approximately 30 FPS to avoid overwhelming the GUI
     */
    void updateViz(const std::vector<util::Position>& positions, double score);

    /**
     * @brief Keep the visualization alive for a short period to allow GUI event processing
     * @param pause_time Time in seconds to keep alive
     */
    void keepAlive(double pauseTime = 0.1);

    /**
     * @brief Save the current figures to files
     * @param gridFilename Filename for the grid visualization
     * @param graphFilename Filename for the graph visualization
     * @param statsFilename Filename for the stats visualization
     */
    void saveFigures(const std::string& gridAnimationFilename = "grid.gif",
        const std::string& gridStaticFilename = "grid.png",
        const std::string& graphFilename = "graph.png",
        const std::string& statsFilename = "stats.png");

    /**
     * @brief Callback when the Python visualization window is closed by the user
     * Note: This is called from the Python thread
     */
    void onWindowCloseCallback();

    /**
     * @brief Shutdown the visualization system and cleanup threads
     */
    void shutdown();

private:
    PyObject* pVizModule = nullptr;
    PyObject* pVizClass = nullptr;
    PyObject* pVizInstance = nullptr;
    PyObject* pVizUpdateClass = nullptr;

    int gridRows = 0;
    int gridCols = 0;
    int numVertices = 0;
    int plotTypes = 0;
    bool initialized = false;

    std::vector<int> offsets;
    std::vector<int> neighbors;
    std::vector<util::Position> lastPositions;

    // Store main thread state for proper GIL management
    PyThreadState* mainThreadState = nullptr;

    /**
     * @brief Create a Python callable for the window close callback
     * @return New reference to a PyObject callable
     */
    PyObject* createOnCloseCallback();

    /**
     * @brief Initialize the Python interpreter and import necessary modules
     * @return true if successful
     * @note Must be called before any other Python operations
     */
    bool initializePython();

    /**
     * @brief Create an instance of the visualization class in Python
     * @param plotTypes Bitmask of plot types to enable (grid, graph, stats)
     * @return true if successful
     */
    bool createVizInstance(int plotTypes);

    /**
     * @brief Initialize the grid visualization in Python
     * @param rows Number of grid rows
     * @param cols Number of grid columns
     * @param numVertices Number of vertices in the graph
     * @return true if successful
     */
    bool initGrid(int rows, int cols, int numVertices);

    /**
     * @brief Initialize the graph visualization in Python
     * @param offsets Adjacency list offsets
     * @param neighbors Adjacency list neighbors
     * @return true if successful
     */
    bool initGraph(const std::vector<int>& offsets, const std::vector<int>& neighbors);

    /**
     * @brief Initialize the stats visualization in Python
     * @return true if successful
     */
    bool initStats();

    /**
     * @brief Update thread information for synchronization
     * @param updateQueue Reference to the visualization update queue
     * @param queueMutex Mutex for synchronizing access to the update queue
     * @param queueCondition Condition variable to signal new updates
     * @param shouldStop Atomic flag to signal shutdown
     */
    void cleanupPython();
};

/**
 * @brief Run the visualization loop on the main thread.
 * @param updateQueue Thread-safe queue of visualization updates
 * @param queueMutex Mutex for synchronizing access to the update queue
 * @param queueCondition Condition variable to signal new updates
 * @param shouldStop Atomic flag to signal shutdown
 */
void visualizationLoop(PyVisualizer& viz);

}; // namespace viz

#else

#include <iostream>

#endif // HAVE_PYTHON

#endif // PYVIZ_HPP
