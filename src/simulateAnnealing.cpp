
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "Graph.hpp"
#include "simulateAnnealing.hpp"

#define INITIAL_TEMPERATURE 10000000.0f
#define COOLING_RATE 0.9999f
#define THRESHOLD_TEMPERATURE 1.0f

/**
 * @brief Alters the current solution by swapping the positions of two randomly selected vertices.
 * @param graph The graph whose vertex positions will be altered.
 * @param generator A random number generator.
 * @param vertex_distribution A uniform integer distribution to select vertex indices.
 * @return A pair of integers representing the indices of the swapped vertices.
 */
std::pair<int, int> alter_solution(Graph& graph, std::mt19937& generator,
    std::uniform_int_distribution<int>& srcVertexDistribution,
    std::uniform_int_distribution<int>& dstVertexDistribution)
{
    int srcVertex = srcVertexDistribution(generator);
    int dstVertex = dstVertexDistribution(generator);

    // Make sure we don't self-swap by pushing the second vertex index up if necessary
    if (dstVertex >= srcVertex)
        dstVertex++;

    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    std::swap(positions[srcVertex], positions[dstVertex]);

    return { srcVertex, dstVertex };
}

/**
 * @brief Reverts the alteration made by swapping two vertices back to their original positions.
 * @param graph The graph whose vertex positions will be reverted.
 * @param srcVertex The index of the first vertex that was swapped.
 * @param dstVertex The index of the second vertex that was swapped.
 */
void revertAlteration(Graph& graph, int srcVertex, int dstVertex)
{
    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    std::swap(positions[srcVertex], positions[dstVertex]);
}

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that determine what additional
 * features to use.
 */
void simulateAnnealing(Graph& graph, const std::vector<std::string>& flags)
{

    /*
    Pseudocode for Simulated Annealing:
    Begin
        generate initial solution
        score initial solution
        set initial temperature (T)
        Loop
            generate new solution
            score new solution
            If new better than old
                replace old solution with new
            Else
                compute ΔE (ΔE = |scoreold − scorenew|)
                compute acceptance probability (p = e-ΔE/T)
                generate random probability (r)
                If (r ≤ p)
                    replace old solution with new
                    EndIf
                EndIf
            lower T
        EndLoop when T is below threshold
    End
    */

    // std::random_device randomSeed;
    // std::mt19937 generator(randomSeed());
    std::mt19937 generator(0); // For reproducibility during testing
    std::uniform_real_distribution<double> probability_distribution(0.0, 1.0);
    std::uniform_int_distribution<int> srcVertexDistribution(0, graph.getNumVertices() - 1);
    // Second distribution to ensure we don't self swap
    std::uniform_int_distribution<int> dstVertexDistribution(0, graph.getNumVertices() - 2);

    // Get an initial solution (it'll just be sequential placement on the grid for now)
    graph.initializeVertexPositions();
    int lastUsedDistance = graph.scoreGraphLayout();

    // Use this to track the best positions found so far to make sure we're not potentially losing a
    // better position when the randomness of the algorithm kicks in
    std::vector<util::Position> lastBestPositions = graph.getCopyVertexPositions();
    double temperature = INITIAL_TEMPERATURE;
    int iteration = 0;
    while (temperature > THRESHOLD_TEMPERATURE) {
        // Generate a new solution by randomly swapping two vertex positions
        std::pair<int, int> swappedVertices
            = alter_solution(graph, generator, srcVertexDistribution, dstVertexDistribution);
        int newDistance = graph.scoreGraphLayout(); // Consider caching and updating on swaps later
        // If our distance is smaller, we have a better solution, so keep it
        if (newDistance < lastUsedDistance) {
            lastUsedDistance = newDistance;
            // We only save the best positions if we've actually improved
            lastBestPositions = graph.getCopyVertexPositions();
        } else {
            // If we didn't improve, we might still accept the new position with some probability
            int deltaE = std::abs(lastUsedDistance - newDistance);
            double acceptanceProbability = std::exp(-static_cast<double>(deltaE) / temperature);
            double randomProbability = probability_distribution(generator);
            // Here, we accept the new solution, but don't update the best known positions
            if (randomProbability <= acceptanceProbability) {
                lastUsedDistance = newDistance;
            }
            // If we don't accept the new solution, revert to the last best known positions
            // This isn't strictly part of the algorithm, but due to my in place alteration to avoid
            // copying the entire position vector, I need to do this to ensure I don't use a
            // solution that I've already rejected
            else {
                revertAlteration(graph, swappedVertices.first, swappedVertices.second);
            }
        }
        temperature *= COOLING_RATE; // Cool down the system
        iteration++; // Just used to debug/report
    }
    // At the end, make sure we have the best positions found during the entire process
    graph.getVertexPositionsRef() = lastBestPositions;

    std::cout << "Completed Simulated Annealing" << std::endl;
    std::cout << "Run " << iteration << " iterations." << std::endl;
}