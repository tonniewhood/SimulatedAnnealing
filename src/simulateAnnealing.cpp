
#include <cctype>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Graph.hpp"
#include "pyViz.hpp"
#include "simulateAnnealing.hpp"
#include "util.hpp"

#define INITIAL_TEMPERATURE 10000000.0f
#define COOLING_RATE 0.99999f
#define THRESHOLD_TEMPERATURE 1.0f

using steadyClock = std::chrono::steady_clock;
using namespace std::chrono_literals;
constexpr double target_fps = 10.0;
constexpr auto frame_dt = 1.0s / target_fps;

namespace sim {

/**
 * @brief Converts a mutation method into the corresponding string value
 * @param method the enum element describing the MutationMethod
 * @return The string name of the enum
 */
std::string mutationMethodToString(MutationMethod method)
{
    switch (method) {
    case NAIVE:
        return std::string("NAIVE");
    case CONWAY:
        return std::string("CONWAY");
    case UNDEFINED:
        return std::string("UNDEFINED");
    }

    return std::string("UNKNOWN");
}

/**
 * @brief Converts a string into a MutationMethod enum. If the string is unknown, this returns
 * UNDEFIND
 * @param methodStr the string corresponding to the Mutation Method
 * @return The enum element
 */
MutationMethod stringToMutationMethod(const std::string& methodStr)
{
    if (util::toLower(methodStr) == "naive")
        return NAIVE;
    if (util::toLower(methodStr) == "conway")
        return CONWAY;
    return UNDEFINED;
}

/**
 * @brief Alters the current solution by swapping the positions of two randomly selected vertices.
 * @param graph The graph whose vertex positions will be altered.
 * @param generator A random number generator.
 * @param srcDistribution A uniform integer distribution to select source vertex indices.
 * @param dstDistribution A uniform integer distribution to select destination vertex indices.
 * @return A pair of integers representing the indices of the swapped vertices.
 */
SolutionAlterations naiveNeighbor(Graph& graph, std::mt19937& generator,
    std::uniform_int_distribution<int>& srcVertexDistribution,
    std::uniform_int_distribution<int>& dstVertexDistribution,
    std::uniform_real_distribution<double>& /*probabilityDistribution*/)
{
    int srcVertex = srcVertexDistribution(generator);
    int dstVertex = dstVertexDistribution(generator);

    // Make sure we don't self-swap by pushing the second vertex index up if necessary
    if (dstVertex >= srcVertex)
        dstVertex++;

    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    util::Position srcPos = positions[srcVertex], dstPos = positions[dstVertex];
    std::swap(positions[srcVertex], positions[dstVertex]);

    // TODO: Potentially adjust the scores in place rather than having to recompute over and over.

    return SolutionAlterations(srcVertex, dstVertex, srcPos, dstPos);
}

/**
 * @brief Reverts the alteration made by swapping two vertices back to their original positions.
 * @param graph The graph whose vertex positions will be reverted.
 * @param alterations The alterations made to the graph to get the previous solution
 */
void revertNaive(Graph& graph, const SolutionAlterations& alterations)
{
    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    std::swap(positions[alterations.vertices.src], positions[alterations.vertices.dst]);

    // TODO: You'd need to revert this here if we do in place score calculation
}

/**
 * @brief Alters the current solution by using a "Conways-game-of-life-esque" idea (Credit to
 * Jayse Hall for the concept and name), to padd the valid region to allow for shape transformation
 * rather than keeping a fixed shape that was initially made.
 * @param graph The graph whose vertex positions will be altered.
 * @param generator A random number generator.
 * @param srcDistribution A uniform integer distribution to select source vertex indices.
 * @param dstDistribution A uniform integer distribution to select destination vertex indices.
 */
SolutionAlterations conwayNeighbor(Graph& graph, std::mt19937& generator,
    std::uniform_int_distribution<int>& srcVertexDistribution,
    std::uniform_int_distribution<int>& dstVertexDistribution,
    std::uniform_real_distribution<double>& probabilityDistribution)
{
    // Resize the distributions if neccessary
    if (dstVertexDistribution.max() != graph.getNumVertices() + graph.getNumPaddedPositions() - 2) {
        // Update the distributions to account for padded positions
        int totalPositions = graph.getNumVertices() + graph.getNumPaddedPositions();
        dstVertexDistribution.param(
            std::uniform_int_distribution<int>::param_type(0, totalPositions - 2));
    }

    int srcVertex = srcVertexDistribution(generator);
    int dstVertex = dstVertexDistribution(generator);

    // Make sure we don't self-swap by pushing the second vertex index up if necessary
    if (dstVertex >= srcVertex)
        dstVertex++;

    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    std::vector<util::Position>& padding = graph.getPaddingPositionsRef();

    util::Position srcPos = positions[srcVertex];
    util::Position dstPos;
    if (dstVertex >= graph.getNumVertices()) {
        double swapProb = probabilityDistribution(generator);
        double ratio = static_cast<double>(graph.getNumVertices())
            / static_cast<double>(graph.getNumPaddedPositions());
        double swapThreshold = 0.5 * ((ratio <= 0.25) ? 1.0 : (0.25 / ratio));

        // If we have a high enough probability, we swap with the padded position
        if (swapProb < swapThreshold) {
            int padIdx = dstVertex - graph.getNumVertices();
            dstPos = padding[padIdx];
            positions[srcVertex] = dstPos;
            graph.updatePadding(srcPos, dstPos);

            return SolutionAlterations(srcVertex, -1, srcPos, dstPos);
        }

        // If we didn't swap with a padded position, we can just return the src vertex and position
        // for both
        return SolutionAlterations(srcVertex, srcVertex, srcPos, srcPos);
    }

    dstPos = positions[dstVertex];
    std::swap(positions[srcVertex], positions[dstVertex]);

    return SolutionAlterations(srcVertex, dstVertex, srcPos, dstPos);
}

/**
 * @brief Reverts the solution of a conway altered graph
 * @param graph The graph whose vertex positions will be altered
 * @param alterations The alterations made to get the previous solution
 */
void revertConway(Graph& graph, const SolutionAlterations& alterations)
{
    std::vector<util::Position>& positions = graph.getVertexPositionsRef();

    if (alterations.vertices.dst == -1) {
        // If the dst vertex is -1, it means we swapped with a padded position
        positions[alterations.vertices.src] = alterations.positions.srcPos;
        graph.updatePadding(alterations.positions.dstPos, alterations.positions.srcPos);
    } else {
        std::swap(positions[alterations.vertices.src], positions[alterations.vertices.dst]);
    }
}

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that determine what additional
 * features to use.
 */
void simulateAnnealing(
    Graph& graph, MutationMethod method, bool sendUpdates, viz::ThreadControlPtr vizThreadControls)
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
    std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);
    MutationFunction mutationMethod;
    RestoreFunction restoreMethod;
    std::uniform_int_distribution<int> srcVertexDistribution(0, graph.getNumVertices() - 1);
    std::uniform_int_distribution<int> dstVertexDistribution(0, graph.getNumVertices() - 2);
    switch (method) {
    case NAIVE:
        mutationMethod = naiveNeighbor;
        restoreMethod = revertNaive;
        break;
    case CONWAY:
        mutationMethod = conwayNeighbor;
        restoreMethod = revertConway;
        break;
    case UNDEFINED:
        std::cerr << "Error: Unknown mutation method." << std::endl;
        return;
    }

    // ---- Used if we're plotting, but we still need he handles inside the loop ----
    auto next_frame = steadyClock::now() + frame_dt;

    std::cout << "Starting Simulated Annealing with method: " << mutationMethodToString(method)
              << std::endl;

    // Get an initial solution (it'll just be sequential placement on the grid for now)
    graph.initializeVertexPositions();

    int lastUsedDistance = graph.scoreGraphLayout();

    // Use this to track the best positions found so far to make sure we're not potentially
    // losing a better position when the randomness of the algorithm kicks in
    std::vector<util::Position> lastBestPositions = graph.getCopyVertexPositions();
    int lastBestDistance = lastUsedDistance;
    double temperature = INITIAL_TEMPERATURE;
    int iteration = 0;
    while (temperature > THRESHOLD_TEMPERATURE) {
        // Generate a new solution by randomly swapping two vertex positions
        SolutionAlterations alterations = mutationMethod(graph, generator, srcVertexDistribution,
            dstVertexDistribution, probabilityDistribution);
        int newDistance = graph.scoreGraphLayout(); // Consider caching and updating on swaps later
        // If our distance is smaller, we have a better solution, so keep it
        if (newDistance < lastUsedDistance) {
            lastUsedDistance = newDistance;
            // We only save the best positions if we've actually improved
            if (lastUsedDistance < lastBestDistance) {
                lastBestPositions = graph.getCopyVertexPositions();
                lastBestDistance = lastUsedDistance;
            }
        } else {
            // If we didn't improve, we might still accept the new position with some
            // probability
            int deltaE = std::abs(lastUsedDistance - newDistance);
            double acceptanceProbability = std::exp(-static_cast<double>(deltaE) / temperature);
            double randomProbability = probabilityDistribution(generator);
            // Here, we accept the new solution, but don't update the best known positions
            if (randomProbability <= acceptanceProbability) {
                lastUsedDistance = newDistance;
            }
            // If we don't accept the new solution, revert to the last best known positions
            // This isn't strictly part of the algorithm, but due to my in place alteration to
            // avoid copying the entire position vector, I need to do this to ensure I don't use
            // a solution that I've already rejected
            else {
                restoreMethod(graph, alterations);
            }
        }

        if (sendUpdates && vizThreadControls && steadyClock::now() >= next_frame) {
            // If we have visualization controls, send an update
            viz::VizUpdate update;
            update.score = lastUsedDistance;
            update.positions = graph.getCopyVertexPositions();

            {
                while (!vizThreadControls->queueMutex.try_lock())
                    std::this_thread::sleep_for(1ms);
                vizThreadControls->messageQueue.push(update);
                vizThreadControls->queueMutex.unlock();
            }
            vizThreadControls->queueCondition.notify_one();

            next_frame += frame_dt;
        }

        temperature *= COOLING_RATE; // Cool down the system
        iteration++; // Just used to debug/report
    }
    // At the end, make sure we have the best positions found during the entire process
    graph.getVertexPositionsRef() = lastBestPositions;

    std::cout << "Completed Simulated Annealing" << std::endl;
    std::cout << "Run " << iteration << " iterations." << std::endl;

    // Send the final state, and then let the user close the viz windows
    if (sendUpdates) {
        viz::VizUpdate update;
        update.score = lastUsedDistance;
        update.positions = graph.getCopyVertexPositions();

        {
            while (!vizThreadControls->queueMutex.try_lock())
                std::this_thread::sleep_for(1ms);
            vizThreadControls->messageQueue.push(update);
            vizThreadControls->queueMutex.unlock();
        }
        vizThreadControls->queueCondition.notify_one();

        std::cout << "Close the visualization windows to exit." << std::endl;
    }
}
}; // namespace sim
