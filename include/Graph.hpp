
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

/**
 * @brief A simple Graph class to carry out graph-related operations. It's built more as an
 * interface for the graph rather than actually implementing the annealing algorithm here.
 */
class Graph {

public:
    Graph(const std::filesystem::path& inputFilePath, const std::filesystem::path& outputFilePath);
    ~Graph();

    /**
     * @brief Reads the input file specified by inputFilePath and processes its contents.
     */
    void readInputFile();

private:
    /**
     * @brief Validates and extracts the header information from the input file.
     *
     * @param inputFile The input file stream.
     * @param line A string to hold the current line being processed.
     * @param lineStream A stringstream to parse the current line.
     */
    void validateHeaderInfo(
        std::ifstream& inputFile, std::string& line, std::stringstream& lineStream);

    /**
     * @brief Validates and extracts edge information from the input file.
     *
     * @param inputFile The input file stream.
     * @param line A string to hold the current line being processed.
     * @param lineStream A stringstream to parse the current line.
     * @param edges A vector of vectors to store the edges.
     * @param maxVertexIndex An integer to track the maximum vertex index seen.
     */
    void validateEdgeInfo(std::ifstream& inputFile, std::string& line,
        std::stringstream& lineStream, std::vector<std::vector<int>>& edges, int& maxVertexIndex);

    /**
     * @brief Reads the graph edges from the input file and returns them as a vector of pairs. Each
     * pair represents an edge between two vertices.
     *
     * @return std::vector<std::vector<int>> A vector of pairs representing the graph edges.
     */
    std::vector<std::vector<int>> getGraphEdges();

    std::filesystem::path inputFilePath;
    std::filesystem::path outputFilePath;

    int gridWidth;
    int gridHeight;
    int numVertices;

    /**
     * I want to use a compressed sparse row (CSR) format to represent the graph.
     * As I understand it, this involves two main arrays:
     * 1. Offsets Array: This array indicates where the neighbors of each vertex start (implying
     *  that the proceeding index is the exclusive end of that vertex's neighbors).
     * 2. Neighbors Array: This array contains all the neighboring vertices for each vertex.
     *
     * So an example might be:
     *
     * Vertex 0 has neighbors 1, 2
     * Vertex 1 has neighbor 2
     * Vertex 2 has no neighbors
     *
     * Offsets: {0, 2, 3, 3}
     * Neighbors: {1, 2, 2}
     *
     * This shows that vertex 0 has 2 neighbors, as offsets[1] - offsets[0] = 2 - 0 = 2
     * Using that offset information, we can determine that indeces 0 and 1 correspond to neighbors
     * of vertex 0. From that, we can see that vertex 0 has neighbors 1 and 2.
     *
     * This method seems to work for both directed and undirected graphs, as long as the edges are
     * added appropriately.
     */
    std::vector<int> offsets;
    std::vector<int> neighbors;
};

#endif // GRAPH_HPP
