

#include "pyViz.hpp"

#include <cctype>
#include <chrono>
#include <cmath>
#include <iostream>
#include <queue>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Graph.hpp"
#include "simulateAnnealing.hpp"
#include "util.hpp"

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
    case SHIFT:
        return std::string("SHIFT");
    case CENTROID:
        return std::string("CENTROID");
    case UNDEFINED:
        return std::string("UNDEFINED");
    }

    // Should never get here, but I hate compiler warnings
    return std::string("UNKNOWN");
}

/**
 * @brief Converts a string into a MutationMethod enum. If the string is unknown, this returns
 * UNDEFINED
 * @param methodStr the string corresponding to the Mutation Method
 * @return The enum element
 */
MutationMethod stringToMutationMethod(const std::string& methodStr)
{
    std::cout << "Using mutation method: " << methodStr << std::endl;

    if (util::toLower(methodStr) == "naive")
        return NAIVE;
    if (util::toLower(methodStr) == "conway")
        return CONWAY;
    if (util::toLower(methodStr) == "shift")
        return SHIFT;
    if (util::toLower(methodStr) == "centroid")
        return CENTROID;
    return UNDEFINED;
}

/**
 * @brief Alters the current solution by swapping the positions of two randomly selected vertices.
 * @param graph The graph whose vertex positions will be altered.
 * @param generator A random number generator.
 * @param srcDistribution A uniform integer distribution to select source vertex indices.
 * @param dstDistribution A uniform integer distribution to select destination vertex indices.
 * @return A SolutionAlterations object representing the vertices and their original positions that were swapped.
 */
SolutionAlterations naiveNeighbor(Graph& graph, std::mt19937& generator)
{
    static std::uniform_int_distribution<int> srcVertexDistribution(0, graph.getNumVertices() - 1);
    if (srcVertexDistribution.max() != graph.getNumVertices() - 1) {
        srcVertexDistribution.param(std::uniform_int_distribution<int>::param_type(0, graph.getNumVertices() - 1));
    }
    static std::uniform_int_distribution<int> dstVertexDistribution(0, graph.getNumVertices() - 2);
    if (dstVertexDistribution.max() != graph.getNumVertices() - 2) {
        dstVertexDistribution.param(std::uniform_int_distribution<int>::param_type(0, graph.getNumVertices() - 2));
    }

    int srcVertex = srcVertexDistribution(generator);
    int dstVertex = dstVertexDistribution(generator);

    // Make sure we don't self-swap by pushing the second vertex index up if necessary
    if (dstVertex >= srcVertex)
        dstVertex++;

    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    util::Position srcPos = positions[srcVertex], dstPos = positions[dstVertex];
    std::swap(positions[srcVertex], positions[dstVertex]);

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
}

/**
 * @brief Alters the current solution by using a "Conways-game-of-life-esque" idea (Credit to
 * Jayse Hall for the concept and name), to padd the valid region to allow for shape transformation
 * rather than keeping a fixed shape that was initially made.
 * @param graph The graph whose vertex positions will be altered.
 * @param generator A random number generator.
 * @param srcDistribution A uniform integer distribution to select source vertex indices.
 * @param dstDistribution A uniform integer distribution to select destination vertex indices.
 * @param probabilityDistribution A uniform real distribution to determine if we swap with a padded
 * position or not.
 * @return A SolutionAlterations object representing the vertices and their original positions that were swapped.
 * is -1, it means we swapped with a padded position.
 */
SolutionAlterations conwayNeighbor(Graph& graph, std::mt19937& generator)
{
    static std::uniform_int_distribution<int> srcVertexDistribution(0, 0);
    if (srcVertexDistribution.max() != graph.getNumVertices() - 1) {
        srcVertexDistribution.param(std::uniform_int_distribution<int>::param_type(0, graph.getNumVertices() - 1));
    }
    static std::uniform_int_distribution<int> dstVertexDistribution(0, 0);
    if (dstVertexDistribution.max() != graph.getNumVertices() + graph.getNumPaddedPositions() - 2) {
        // Update the distributions to account for padded positions
        int totalPositions = graph.getNumVertices() + graph.getNumPaddedPositions();
        dstVertexDistribution.param(std::uniform_int_distribution<int>::param_type(0, totalPositions - 2));
    }
    static std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);

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
        double ratio = static_cast<double>(graph.getNumVertices()) / static_cast<double>(graph.getNumPaddedPositions());
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
 * @brief Alters the current solution by trying to place a vertex in a position to minimize the largest
 * edge distance. If the space is filled, the vertex is shifted out and the process repeats.
 * @param graph The graph whose vertex positions will be altered.
 * @param generator A random number generator.
 * @param srcDistribution A uniform integer distribution to select source vertex indices.
 * @param probabilityDistribution A uniform real distribution to determine if we overwrite a filled
 * position or not.
 * @return A SolutionAlterations object representing the vertices and their original positions that were swapped. If the
 * dst vertex is -1, it means we put into an empty space.
 */
SolutionAlterations shiftNeighbor(Graph& graph, std::mt19937& generator)
{
    static std::uniform_int_distribution<int> srcVertexDistribution(0, 0);
    if (srcVertexDistribution.max() != graph.getNumVertices() - 1) {
        srcVertexDistribution.param(std::uniform_int_distribution<int>::param_type(0, graph.getNumVertices() - 1));
    }
    static std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);
    static int maxSlides = static_cast<int>(std::max(graph.getGridWidth(), graph.getGridHeight()) / 2);

    int srcVertex = srcVertexDistribution(generator);
    std::vector<util::Position>& positions = graph.getVertexPositionsRef();
    auto [furthestNeighbor, neighborPos] = graph.getFurthestNeighbor(srcVertex);

    int lastValidVertex = srcVertex;
    util::Position srcPos = positions[srcVertex];
    util::Position lastValidPos = positions[srcVertex];
    std::unordered_set<util::Position> visitedPositions = { srcPos };
    for (int slides = 1; slides <= maxSlides; slides++) {
        std::vector<util::Position> validAdjacent = graph.getValidAdjacentCells(neighborPos, visitedPositions);

        // We shouldn't need to check against max slides
        if (validAdjacent.empty() || slides > maxSlides) {
            return SolutionAlterations(srcVertex, srcVertex, srcPos, srcPos);
        }

        std::uniform_int_distribution<int> adjDistribution(0, validAdjacent.size() - 1);
        util::Position newPos = validAdjacent[adjDistribution(generator)];
        if (!graph.isPositionOccupied(newPos)) {
            graph.updateVertexPositions(srcVertex, -1, srcPos, newPos);
            return SolutionAlterations(srcVertex, -1, srcPos, newPos);
        }

        // If the new position is occupied, we need to get a random probability to see if we
        // overwrite it or not
        neighborPos = newPos;
        lastValidVertex = graph.getVertexAtPosition(newPos);
        lastValidPos = newPos;
        double overwriteThreshold = 0.5 + (0.5 * static_cast<double>(slides)) / static_cast<double>(maxSlides);
        double overwriteProb = probabilityDistribution(generator);
        if (overwriteProb < overwriteThreshold) {
            graph.updateVertexPositions(srcVertex, lastValidVertex, srcPos, newPos);
            return SolutionAlterations(srcVertex, lastValidVertex, srcPos, newPos);
        }

        // Insert the position we attempted to move to into the visited set so we don't
        // immediately try to move back to it
        visitedPositions.insert(newPos);
    }

    std::cerr << "Unexpected region reached in shiftNeighbor. Delaying for debugging." << std::endl;
    std::this_thread::sleep_for(50ms);
    // Realistically, we should never get here, but if we do, just return no change
    return SolutionAlterations(srcVertex, srcVertex, srcPos, srcPos);
}

/**
 * @brief Reverts the solution of a shift altered graph
 * @param graph The graph whose vertex positions will be altered
 * @param alterations The alterations made to get the previous solution
 */
void revertShift(Graph& graph, const SolutionAlterations& alterations)
{
    if (alterations.vertices.dst == -1) {
        // If the dst vertex is -1, it means we put into an empty space
        graph.updateVertexPositions(
            alterations.vertices.src, -1, alterations.positions.dstPos, alterations.positions.srcPos);
    } else {
        graph.updateVertexPositions(alterations.vertices.src, alterations.vertices.dst, alterations.positions.dstPos,
            alterations.positions.srcPos);
    }
}

/**
 * @brief Alters the current solution by taking a random vertex, and determines the "centroid" it's neighbors as a
 * polygon. It then attempts to place the vertex in the centroid position, swapping with any vertices in that position
 * (if any)
 * @param graph The graph whose vertex positions will be altered.
 * @param generator The random number generator to use for sampling.
 * @return A SolutionAlterations object representing the vertices and their original positions that were swapped. If the
 * dst vertex is -1, it means we put into an empty space.
 */
SolutionAlterations centroidNeighbor(Graph& graph, std::mt19937& generator)
{
    static std::uniform_int_distribution<int> srcVertexDistribution(0, 0);
    if (srcVertexDistribution.max() != graph.getNumVertices() - 1) {
        srcVertexDistribution.param(std::uniform_int_distribution<int>::param_type(0, graph.getNumVertices() - 1));
    }
    static std::uniform_int_distribution<int> push_direction(-1, 1);
    static std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);
    int rowPushDir = push_direction(generator);
    int colPushDir = push_direction(generator);

    std::vector<util::Position>& positions = graph.getVertexPositionsRef();

    int srcVertex = srcVertexDistribution(generator);
    util::Position srcPos = positions[srcVertex];
    auto [occupyingVertex, centroidPos] = graph.getCentroidPosition(srcVertex, rowPushDir, colPushDir);

    if (!graph.isPositionOccupied(centroidPos)) {
        graph.updateVertexPositions(srcVertex, -1, srcPos, centroidPos);
        return SolutionAlterations(srcVertex, -1, srcPos, centroidPos);
    }

    double overwriteThreshold
        = 0.6; // This is 100% a Magic Number and I'm not afraid to admit that. It came to me in a delusion
    double overwriteProb = probabilityDistribution(generator);
    if (overwriteProb < overwriteThreshold) {
        graph.updateVertexPositions(srcVertex, graph.getVertexAtPosition(centroidPos), srcPos, centroidPos);
        return SolutionAlterations(srcVertex, graph.getVertexAtPosition(centroidPos), srcPos, centroidPos);
    }

    // If we didn't overwrite, just return no change
    return SolutionAlterations(srcVertex, srcVertex, srcPos, srcPos);
}

/**
 * @brief Reverts the solution of a centroid altered graph
 * @param graph The graph whose vertex positions will be altered
 * @param alterations The alterations made to get the previous solution
 */
void revertCentroid(Graph& graph, const SolutionAlterations& alterations)
{
    if (alterations.vertices.dst == -1) {
        // If the dst vertex is -1, it means we put into an empty space
        graph.updateVertexPositions(
            alterations.vertices.src, -1, alterations.positions.dstPos, alterations.positions.srcPos);
    } else {
        graph.updateVertexPositions(alterations.vertices.src, alterations.vertices.dst, alterations.positions.dstPos,
            alterations.positions.srcPos);
    }
}

static const MutationFunction mutationMethods[] = { naiveNeighbor, conwayNeighbor, shiftNeighbor, centroidNeighbor };
static const RestoreFunction restoreMethods[] = { revertNaive, revertConway, revertShift, revertCentroid };

#if HAVE_PYTHON

#define WINDOW_LEN 5000

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that determine what additional
 * features to use.
 */
int simulateAnnealing(Graph& graph, double startingTemperature, double coolingRate, MutationMethod method,
    bool sendUpdates, viz::ThreadControlPtr vizThreadControls)
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

    std::random_device randomSeed;
    std::mt19937 generator(randomSeed());
    std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);
    if (method < NAIVE || method > CENTROID) {
        std::cerr << "Invalid mutation method specified, defaulting to NAIVE." << std::endl;
        method = NAIVE;
    }

    std::cout << "Starting Simulated Annealing with method: " << mutationMethodToString(method) << std::endl;

    // Get an initial solution (it'll just be sequential placement on the grid for now)
    graph.initializeVertexPositions();

    int lastUsedDistance = graph.scoreGraphLayout();

    // Use this to track the best positions found so far to make sure we're not potentially
    // losing a better position when the randomness of the algorithm kicks in
    std::vector<util::Position> lastBestPositions = graph.getCopyVertexPositions();
    int lastBestDistance = lastUsedDistance;
    double temperature = startingTemperature;
    int iteration = 0;

    // Unused if we're not sending updates
    auto start_time = steadyClock::now();
    auto next_frame = start_time + frame_dt;
    double acceptedScoresSum = 0.;
    int deltaE = 0;
    std::vector<std::chrono::duration<double, std::nano>> timestamps;
    std::vector<double> temperatures;
    std::vector<int> scores;
    std::vector<int> bestScores;
    std::vector<int> scoreDeltas;
    std::vector<double> acceptanceRates;
    std::deque<double> acceptedScores;

    if (sendUpdates) {
        timestamps.reserve(1000); // Arbitrary initial capacity
        temperatures.reserve(1000);
        scores.reserve(1000);
        bestScores.reserve(1000);
        scoreDeltas.reserve(1000);
        acceptanceRates.reserve(1000);
    }

    while (temperature > THRESHOLD_TEMPERATURE) {

        if (vizThreadControls) {
            if (vizThreadControls->shouldStop.load()) {
                std::cout << "Annealing process received stop signal, terminating early." << std::endl;
                vizThreadControls->stoppedEarly.store(true);
                return lastBestDistance;
            }
        }

        // Generate a new solution by randomly swapping two vertex positions
        SolutionAlterations alterations = mutationMethods[method](graph, generator);
        int newDistance = graph.scoreGraphLayout();
        deltaE = lastUsedDistance - newDistance;
        // If our distance is smaller, we have a better solution, so keep it
        if (deltaE > 0) {
            lastUsedDistance = newDistance;
            // We only save the best positions if we've actually improved
            if (lastUsedDistance < lastBestDistance) {
                lastBestPositions = graph.getCopyVertexPositions();
                lastBestDistance = lastUsedDistance;
            }

            if (sendUpdates) {
                acceptedScoresSum += 1.;
                acceptedScores.push_back(1.);
                if (acceptedScores.size() > WINDOW_LEN) {
                    acceptedScoresSum -= acceptedScores.front();
                    acceptedScores.pop_front();
                }
            }
        } else {
            // If we didn't improve, we might still accept the new position with some
            // probability
            double acceptanceProbability = std::exp(-static_cast<double>(std::abs(deltaE)) / temperature);
            double randomProbability = probabilityDistribution(generator);
            // Here, we accept the new solution, but don't update the best known positions
            if (randomProbability <= acceptanceProbability) {
                lastUsedDistance = newDistance;
                if (sendUpdates) {
                    acceptedScoresSum += 1.;
                    acceptedScores.push_back(1.);
                    if (acceptedScores.size() > WINDOW_LEN) {
                        acceptedScoresSum -= acceptedScores.front();
                        acceptedScores.pop_front();
                    }
                }
            }
            // If we don't accept the new solution, revert to the last best known positions
            // This isn't strictly part of the algorithm, but due to my in place alteration to
            // avoid copying the entire position vector, I need to do this to ensure I don't use
            // a solution that I've already rejected
            else {
                restoreMethods[method](graph, alterations);
                if (sendUpdates) {
                    acceptedScores.push_back(0.);
                    if (acceptedScores.size() > WINDOW_LEN) {
                        acceptedScoresSum -= acceptedScores.front();
                        acceptedScores.pop_front();
                    }
                }
            }
        }

        if (sendUpdates && vizThreadControls) {

            if (timestamps.size() >= static_cast<size_t>(static_cast<double>(timestamps.capacity()) * 0.75)) {
                // If we've used up 75% of our capacity, double the size of all the vectors
                timestamps.reserve(timestamps.capacity() * 2);
                temperatures.reserve(temperatures.capacity() * 2);
                scores.reserve(scores.capacity() * 2);
                bestScores.reserve(bestScores.capacity() * 2);
                scoreDeltas.reserve(scoreDeltas.capacity() * 2);
                acceptanceRates.reserve(acceptanceRates.capacity() * 2);
            }

            auto current_time = steadyClock::now();
            double acceptanceRate = acceptedScoresSum / static_cast<double>(acceptedScores.size());

            timestamps.push_back(std::chrono::duration<double, std::nano>(current_time - start_time));
            temperatures.push_back(temperature);
            scores.push_back(lastUsedDistance);
            bestScores.push_back(lastBestDistance);
            scoreDeltas.push_back(deltaE);
            acceptanceRates.push_back(acceptanceRate);

            if (current_time >= next_frame) {
                // If we have visualization controls, send an update
                viz::VizUpdate update;

                // Copy over the buffered data
                update.timeStamps = timestamps;
                update.temperatures = temperatures;
                update.scores = scores;
                update.bestScores = bestScores;
                update.scoreDeltas = scoreDeltas;
                update.acceptanceRates = acceptanceRates;

                // Clear the buffers for the next round
                timestamps.clear();
                temperatures.clear();
                scores.clear();
                bestScores.clear();
                scoreDeltas.clear();
                acceptanceRates.clear();

                update.currentScore = lastUsedDistance;
                update.positions = graph.getCopyVertexPositions();

                {
                    std::unique_lock<std::mutex> lock(vizThreadControls->queueMutex);
                    vizThreadControls->messageQueue.push(update);
                }
                vizThreadControls->queueCondition.notify_one();

                next_frame += frame_dt;
            }
        }

        temperature *= coolingRate; // Cool down the system
        iteration++; // Just used to debug/report
    }
    // At the end, make sure we have the best positions found during the entire process
    graph.getVertexPositionsRef() = lastBestPositions;

    std::cout << "Completed Simulated Annealing" << std::endl;
    std::cout << "Run " << iteration << " iterations." << std::endl;

    // Send the final state, and then let the user close the viz windows
    if (sendUpdates) {
        viz::VizUpdate update;

        // Copy over the buffered data
        update.timeStamps = timestamps;
        update.temperatures = temperatures;
        update.scores = scores;
        update.bestScores = bestScores;
        update.scoreDeltas = scoreDeltas;
        update.acceptanceRates = acceptanceRates;
        update.currentScore = lastUsedDistance;
        update.positions = graph.getCopyVertexPositions();

        {
            std::unique_lock<std::mutex> lock(vizThreadControls->queueMutex);
            vizThreadControls->messageQueue.push(update);
        }
        vizThreadControls->queueCondition.notify_one();

        std::cout << "Close the visualization windows to exit." << std::endl;
    }

    return lastBestDistance;
}

#else

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that determine what additional
 * features to use.
 */
int simulateAnnealing(Graph& graph, double startingTemperature, double coolingRate, MutationMethod method)
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

    std::random_device randomSeed;
    std::mt19937 generator(randomSeed());
    std::uniform_real_distribution<double> probabilityDistribution(0.0, 1.0);
    MutationFunction mutationMethod;
    RestoreFunction restoreMethod;
    if (method < NAIVE || method > CENTROID) {
        std::cerr << "Invalid mutation method specified, defaulting to NAIVE." << std::endl;
        method = NAIVE;
    }

    std::cout << "Starting Simulated Annealing with method: " << mutationMethodToString(method) << std::endl;

    // Get an initial solution (it'll just be sequential placement on the grid for now)
    graph.initializeVertexPositions();

    int lastUsedDistance = graph.scoreGraphLayout();

    // Use this to track the best positions found so far to make sure we're not potentially
    // losing a better position when the randomness of the algorithm kicks in
    std::vector<util::Position> lastBestPositions = graph.getCopyVertexPositions();
    int lastBestDistance = lastUsedDistance;
    double temperature = startingTemperature;
    int iteration = 0;
    while (temperature > THRESHOLD_TEMPERATURE) {

        // Generate a new solution by randomly swapping two vertex positions
        SolutionAlterations alterations = mutationMethods[method](graph, generator);
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
                restoreMethods[method](graph, alterations);
            }
        }

        temperature *= coolingRate; // Cool down the system
        iteration++; // Just used to debug/report
    }
    // At the end, make sure we have the best positions found during the entire process
    graph.getVertexPositionsRef() = lastBestPositions;

    std::cout << "Completed Simulated Annealing" << std::endl;
    std::cout << "Run " << iteration << " iterations." << std::endl;

    return lastBestDistance;
}

#endif // HAVE_PYTHON

/**
 * @brief Runs an indepth analysis on the annealing process, specifcically how the cooling rate
 * affects the final score and time to completion. This will run multiple trials at each
 * cooling rate and record the results. The idea is to also run each of the methods and compare
 * them.
 * @param graph The graph to perform simulated annealing on.
 * @param startingTemperature The starting temperature for the annealing process.
 * @param coolingRates A vector of cooling rates to test.
 * @param numTrials The number of trials to run for each cooling rate.
 * @param outputFilePath The path to the output file where results will be saved.
 * @return The integer exit code. 0 for success, non-zero for failure.
 */
int runAnnealingAnalysis(Graph& graph, double startingTemperature, const std::vector<double>& coolingRates,
    int numTrials, const std::filesystem::path& outputFileDirectory)
{
    std::filesystem::path outputFilePathSummary = outputFileDirectory / std::filesystem::path("annealing_analysis.md");
    std::vector<std::filesystem::path> outputFilePathsCSVs = {
        outputFileDirectory / std::filesystem::path("annealing_analysis_naive.csv"),
        outputFileDirectory / std::filesystem::path("annealing_analysis_conway.csv"),
        outputFileDirectory / std::filesystem::path("annealing_analysis_shift.csv"),
        outputFileDirectory / std::filesystem::path("annealing_analysis_centroid.csv"),
    };

    // Get the required data to store each method's results
    std::vector<MutationMethod> methods = { NAIVE, CONWAY, SHIFT, CENTROID };
    struct MethodStats {
        std::vector<double> coolingRates;
        std::vector<int> bestScores;
        std::vector<int> worstScores;
        std::vector<double> averageScores;
        std::vector<double> fastestTime;
        std::vector<double> slowestTime;
        std::vector<double> averageTime;

        MethodStats(size_t numRates, const std::vector<double>& coolingRates)
            : coolingRates(coolingRates)
            , bestScores(std::vector<int>(numRates, std::numeric_limits<int>::max()))
            , worstScores(std::vector<int>(numRates, std::numeric_limits<int>::min()))
            , averageScores(std::vector<double>(numRates, std::numeric_limits<double>::max()))
            , fastestTime(std::vector<double>(numRates, 0.0))
            , slowestTime(std::vector<double>(numRates, 0.0))
            , averageTime(std::vector<double>(numRates, 0.0)) {};
    };
    std::vector<MethodStats> methodStats;
    methodStats.reserve(methods.size());
    for (size_t i = 0; i < methods.size(); i++) {
        methodStats.emplace_back(coolingRates.size(), coolingRates);
    }

    for (const MutationMethod& method : methods) {
        std::string methodStr = mutationMethodToString(method);
        for (size_t rateIdx = 0; rateIdx < coolingRates.size(); rateIdx++) {
            double coolingRate = coolingRates[rateIdx];
            std::cout << "Running analysis for method " << methodStr << " with cooling rate " << coolingRate
                      << std::endl;
            std::cout << "=================================" << std::endl;

            int bestScore = 0;
            int worstScore = 0;
            double totalScore = 0.0;
            double fastest = 0.0;
            double slowest = 0.0;
            double totalTime = 0.0;
            for (int trial = 1; trial < numTrials + 1; trial++) {
                auto start = steadyClock::now();

                // Suppress stdout during simulateAnnealing
                std::streambuf* orig_buf = std::cout.rdbuf();
                std::ostringstream null_stream;
                std::cout.rdbuf(null_stream.rdbuf());

                int score = simulateAnnealing(graph, startingTemperature, coolingRate, method);

                // Restore stdout
                std::cout.rdbuf(orig_buf);
                auto end = steadyClock::now();

                std::chrono::duration<double> elapsed = end - start;
                totalScore += static_cast<double>(score);
                totalTime += elapsed.count();
                if (trial == 1 || score < bestScore) {
                    bestScore = score;
                }
                if (trial == 1 || score > worstScore) {
                    worstScore = score;
                }
                if (trial == 1 || elapsed.count() < fastest) {
                    fastest = elapsed.count();
                }
                if (trial == 1 || elapsed.count() > slowest) {
                    slowest = elapsed.count();
                }
            }

            double averageScore = totalScore / static_cast<double>(numTrials);
            double averageElapsed = totalTime / static_cast<double>(numTrials);

            methodStats[method].bestScores[rateIdx] = bestScore;
            methodStats[method].worstScores[rateIdx] = worstScore;
            methodStats[method].averageScores[rateIdx] = averageScore;
            methodStats[method].fastestTime[rateIdx] = fastest;
            methodStats[method].slowestTime[rateIdx] = slowest;
            methodStats[method].averageTime[rateIdx] = averageElapsed;
        }

        // Write out the CSV file for this method
        std::ofstream csvFile(outputFilePathsCSVs[method]);
        if (!csvFile.is_open()) {
            std::cerr << "Failed to open output CSV file: " << outputFilePathsCSVs[method] << std::endl;
            return 1;
        }

        csvFile
            << "Cooling Rate,Best Score,Worst Score,Average Score,Fastest Time (s),Slowest Time (s),Average Time (s)\n";
        for (size_t rateIdx = 0; rateIdx < coolingRates.size(); rateIdx++) {
            csvFile << methodStats[method].coolingRates[rateIdx] << "," << methodStats[method].bestScores[rateIdx]
                    << "," << methodStats[method].worstScores[rateIdx] << ","
                    << methodStats[method].averageScores[rateIdx] << "," << methodStats[method].fastestTime[rateIdx]
                    << "," << methodStats[method].slowestTime[rateIdx] << ","
                    << methodStats[method].averageTime[rateIdx] << "\n";
        }

        csvFile.close();
    }

    // Write out the summary file
    std::ofstream summaryFile(outputFilePathSummary);
    if (!summaryFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputFilePathSummary << std::endl;
        return 1;
    }

    std::stringstream summary;
    summary << "# Results of Simulated Annealing Analysis\n\n> N = 10 trials per cooling rate\n\n";
    for (size_t methodIdx = 0; methodIdx < methods.size(); methodIdx++) {
        summary << "### Method: " << mutationMethodToString(methods[methodIdx]) << "\n";
        summary << "|  Cooling Rate   |    Best Score    |    Worst Score   |  Average Score   | Fastest Time (s) | "
                   "Slowest Time (s) | Average Time (s) |\n";
        summary << "|-----------------|------------------|------------------|------------------|------------------|"
                   "------------------|------------------|\n";
        for (size_t rateIdx = 0; rateIdx < coolingRates.size(); rateIdx++) {
            summary << std::left << "|" << std::setw(17) << methodStats[methodIdx].coolingRates[rateIdx] << "|"
                    << std::setw(18) << methodStats[methodIdx].bestScores[rateIdx] << "|" << std::setw(18)
                    << methodStats[methodIdx].worstScores[rateIdx] << "|" << std::setw(18)
                    << methodStats[methodIdx].averageScores[rateIdx] << "|" << std::setw(18)
                    << methodStats[methodIdx].fastestTime[rateIdx] << "|" << std::setw(18)
                    << methodStats[methodIdx].slowestTime[rateIdx] << "|" << std::setw(18)
                    << methodStats[methodIdx].averageTime[rateIdx] << "|\n";
        }
        summary << "\n";
    }

    summaryFile << summary.str();
    summaryFile.close();

    std::cout << "\nAnalysis complete." << std::endl;
    std::cout << "Summary written to " << outputFilePathSummary << std::endl;
    std::cout << "CSV files written to:" << std::endl;
    for (const auto& path : outputFilePathsCSVs) {
        std::cout << "  " << path << std::endl;
    }
    std::cout << "\n\n" << summary.str() << std::endl;

    return 0;
}

}; // namespace sim
