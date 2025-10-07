

#ifndef PYVIZ_HPP
#define PYVIZ_HPP

#include <vector>

#include "util.hpp"

namespace viz {

/**
 * @brief Initialize the Python visualization system
 * @param gridRows Number of rows in the grid
 * @param gridCols Number of columns in the grid
 * @return true if initialization successful
 */
bool initializeVisualizer(int gridRows, int gridCols);

/**
 * @brief Display the graph structure (call once at start)
 * @param edges Vector of edge pairs representing the graph
 */
void displayGraph(const std::vector<int>& offsets, const std::vector<int>& neighbors);

/**
 * @brief Update the grid visualization with current positions
 * @param positions Current vertex positions
 * @param iteration Current iteration number
 * @param score Current annealing score
 * @note Only updates display every 100 iterations to avoid overwhelming
 */
void updateVisualization(const std::vector<util::Position>& positions, int iteration, double score);

/**
 * @brief Shutdown the visualization system and cleanup threads
 */
void shutdownVisualizer();

}; // namespace viz

#endif // PYVIZ_HPP
