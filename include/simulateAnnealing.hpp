
#ifndef SIMULATE_ANNEALING_HPP
#define SIMULATE_ANNEALING_HPP

#include "Graph.hpp"

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that deterimine what additional
 * features to use.
 */
void simulateAnnealing(Graph& graph, const std::vector<std::string>& flags);

#endif // SIMULATE_ANNEALING_HPP
