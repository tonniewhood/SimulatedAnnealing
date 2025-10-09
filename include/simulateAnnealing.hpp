
#ifndef SIMULATE_ANNEALING_HPP
#define SIMULATE_ANNEALING_HPP

#include <functional>
#include <memory>
#include <random>
#include <unordered_map>

#include "Graph.hpp"
#include "util.hpp"

// Forward declare the viz namespace and its contents to avoid including pyViz.hpp here
// It's not necessary if we're not using Python visualization, but the forward declaration
// doesn't hurt anything and the preprocessor instruction blocks are messy
namespace viz {

struct VizUpdate;
using ThreadControlPtr = std::shared_ptr<util::ThreadControls<VizUpdate>>;
}; // namespace viz

namespace sim {

enum MutationMethod { NAIVE, CONWAY, SHIFT, CENTROID, UNDEFINED };

struct SolutionAlterations {
    struct {
        int src, dst;
    } vertices;
    struct {
        util::Position srcPos, dstPos;
    } positions;

    SolutionAlterations(int src, int dst, const util::Position& srcPos, const util::Position& dstPos)
        : vertices({ src, dst })
        , positions({ srcPos, dstPos })
    {
    }
};

using MutationFunction = std::function<SolutionAlterations(Graph& graph, std::mt19937& generator)>;
using RestoreFunction = std::function<void(Graph& graph, const SolutionAlterations& alterations)>;

/**
 * @brief Converts a mutation method into the corresponding string value
 * @param method the enum element describing the MutationMethod
 * @return The string name of the enum
 */
std::string mutationMethodToString(MutationMethod method);

/**
 * @brief Converts a string into a MutationMethod enum. If the string is unknown, this returns
 * UNDEFIND
 * @param methodStr the string corresponding to the Mutation Method
 * @return The enum element
 */
MutationMethod stringToMutationMethod(const std::string& methodStr);

#if HAVE_PYTHON

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that deterimine what additional
 * features to use.
 */
void simulateAnnealing(Graph& graph, double startTemperature, double coolingRate, MutationMethod method = NAIVE,
    bool sendUpdates = false, viz::ThreadControlPtr vizThreadControls = nullptr);

#else

/**
 * @brief Simulates the annealing process on the provided graph using specified flags. It will
 * attempt to place the graph's vertices on a grid in a way that minimizes the square of the
 * distances between connected vertices.
 * @param graph The graph to perform simulated annealing on.
 * @param flags A vector of strings representing various flags that deterimine what additional
 * features to use.
 */
void simulateAnnealing(Graph& graph, double startTemperature, double coolingRate, MutationMethod method = NAIVE);

#endif // HAVE_PYTHON

}; // namespace sim

#endif // SIMULATE_ANNEALING_HPP
