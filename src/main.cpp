
#include <iostream>

#include <filesystem>
#include <string>
#include <vector>

#include "Graph.hpp"
#include "simulateAnnealing.hpp"
#include "util.hpp"

// Fallback in case PROJECT_ROOT is not defined. This allows input files to be found at the project
// root if not specified otherwise.
#ifndef PROJECT_ROOT
#define PROJECT_ROOT "./"
#endif

// To make sure we can reliably find data files, we'll use a DATA_DIR environment variable
#ifndef DATA_DIR
#define DATA_DIR "./data"
#endif

#include <iostream>
/**
 * @brief Scratch space for any dumb test I want to run. It'll keep the main file cleaner.
 * @param graph The graph to perform tests on.
 * @param flags A vector of strings representing various flags that determine what additional
 * features to use.
 */
void runTests(Graph& graph, const std::vector<std::string>& flags)
{
    auto swapFunc = [](Graph& g, int v1, int v2) {
        std::vector<util::Position>& positions = g.getVertexPositionsRef();
        std::swap(positions[v1], positions[v2]);
    };

    if (!graph.initializeVertexPositions()) {
        std::cerr << "Error: Failed to initialize vertex positions." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Initial Graph State:" << std::endl;
    std::cout << "Score: " << graph.scoreGraphLayout() << std::endl;
    std::cout << "Vertex Positions:" << std::endl;
    for (int vertex = 0; vertex < graph.getNumVertices(); vertex++) {
        util::Position pos = graph.getVertexPosition(vertex);
        std::cout << "Node " << vertex << " at (" << pos.row << ", " << pos.col << ")" << std::endl;
    }

    for (int vertex = 0; vertex < graph.getNumVertices() - 1; vertex++) {
        swapFunc(graph, vertex, vertex + 1);
        std::cout << "After swapping vertices " << vertex << " and " << vertex + 1 << ":"
                  << std::endl;
        std::cout << "Score: " << graph.scoreGraphLayout() << std::endl;
        for (int v = 0; v < graph.getNumVertices(); v++) {
            util::Position pos = graph.getVertexPosition(v);
            std::cout << "Node " << v << " at (" << pos.row << ", " << pos.col << ")" << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    // Parse command line arguments
    util::Args args = util::parseCommandLineArgs(argc, argv);

    std::filesystem::path inputFilePath = util::getInputFileLocation(
        args.positional[0], std::string(PROJECT_ROOT), std::string(DATA_DIR));
    std::filesystem::path outputFilePath
        = util::createOutputFilePath(inputFilePath, args.positional[1]);

    // If I don't have a valid input file path, I can't really do anything. So we'll have to quit.
    if (inputFilePath.empty()) {
        std::cerr << "Error: Could not resolve input file path." << std::endl;
        std::cerr << "Found: " << inputFilePath << std::endl;
        return 1;
    }

    // Realistically, this should never fire, but I'd rather make sure to have a valid output path
    // before using it.
    if (outputFilePath.empty()) {
        std::cerr << "Error: Could not create output file path. Defaulting to 'output.txt'."
                  << std::endl;
        outputFilePath = std::filesystem::path("output.txt");
    }

    std::cout << "Using input file: " << inputFilePath << std::endl;
    std::cout << "Using output file: " << outputFilePath << std::endl;

    Graph graph(inputFilePath, outputFilePath);
    graph.readInputFile();

    simulateAnnealing(graph, args.flags);

    if (!graph.reportResults(args.flags)) {
        std::cerr << "Error: Failed to report results." << std::endl;
        return 1;
    }

    return 0;
}