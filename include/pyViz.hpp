

#ifndef PYVIZ_HPP
#define PYVIZ_HPP

#include <vector>

#include "util.hpp"

namespace viz {

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
bool initializeVisualizer(int gridRows, int gridCols, int numVertices,
    const std::vector<int>& offsets, const std::vector<int>& neighbors);

/**
 * @brief Display the graph structure (call once at start)
 */
void displayGraph();

/**
 * @brief Update the grid visualization with current positions
 * @param positions Current vertex positions
 * @param iteration Current iteration number
 * @param score Current annealing score
 * @note Only updates display every 100 iterations to avoid overwhelming
 */
void updateVisualization(const std::vector<util::Position>& positions, int iteration, double score);

/**
 * @brief Check if the visualization windows are still active
 * @return true if active, false if shutdown
 */
bool isActive();

/**
 * @brief Shutdown the visualization system and cleanup threads
 */
void shutdownVisualizer();

}; // namespace viz

#endif // PYVIZ_HPP
