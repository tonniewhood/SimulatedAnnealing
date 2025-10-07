
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <filesystem>
#include <fstream>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "util.hpp"

/**
 * @brief A simple Graph class to carry out graph-related operations. It's built more as an
 * interface for the graph rather than actually implementing the annealing algorithm here.
 */
class Graph {

    typedef std::vector<std::vector<int>> Edges;

public:
    Graph(const std::filesystem::path& inputFilePath, const std::filesystem::path& outputFilePath);
    ~Graph();

    /**
     * @brief Reads the input file specified by inputFilePath and processes its contents.
     */
    void readInputFile();

    /**
     * @brief Initializes vertex positions. If no positions are provided, vertices are placed in
     * order on a grid of size gridWidth x gridHeight.
     * @param positions Optional vector of positions to initialize the vertices. Repeated positions
     * are not allowed.
     * @return true if initialization is successful, false otherwise.
     */
    bool initializeVertexPositions(const std::vector<util::Position>& positions = {});

    /**
     * @brief Retrieves the neighbors of a given vertex.
     * @param vertex The vertex for which to retrieve neighbors.
     * @return A vector of neighboring vertices.
     */
    std::vector<int> getVertexNeighbors(int vertex) const;

    /**
     * @brief Retrieves the position of a given vertex.
     * @param vertex The vertex for which to retrieve the position.
     * @return The position of the vertex.
     */
    util::Position getVertexPosition(int vertex) const;

    /**
     * @brief Updates the padded pixels after a swap has been made.
     * @param vacatedPos The position that was vacated by the swap
     * @param filledPos The position that was filled after the swap
     */
    void updatePadding(util::Position vacatedPos, util::Position filledPos);

    /**
     * @brief Scores the current layout of the graph based on the sum of squared
     * distances between connected vertices.
     * @return The score of the current graph layout.
     */
    int scoreGraphLayout() const;

    /**
     * @brief Reports the results of the graph layout based on input flags
     * @param flags A vector of strings representing various flags that determine what additional
     * information to include in the report.
     * @return bool indicating success or failure of the report operation.
     */
    bool reportResults(const std::unordered_map<std::string, std::string>& flags) const;

    /* Getters for various member variables */
    util::GridDimensions getGridDimensions() const { return this->gridDimensions; }
    int getGridWidth() const { return this->gridDimensions.width; }
    int getGridHeight() const { return this->gridDimensions.height; }
    int getNumVertices() const { return this->numVertices; }
    int getNumPaddedPositions() const { return static_cast<int>(this->paddedPositions.size()); }
    std::vector<int> getOffsets() const { return this->offsets; }
    std::vector<int> getNeighbors() const { return this->neighbors; }
    std::vector<util::Position> getConstVertexPositions() const { return this->vertexPositions; }
    std::vector<util::Position> getCopyVertexPositions() const { return this->vertexPositions; }
    std::vector<util::Position>& getVertexPositionsRef() { return this->vertexPositions; }
    std::vector<util::Position>& getPaddingPositionsRef() { return this->paddedPositions; }

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
        std::stringstream& lineStream, Edges& forwardEdges, Edges& reverseEdges,
        int& maxVertexIndex);

    /**
     * @brief Reads the graph edges from the input file and returns them as a vector of pairs. Each
     * pair represents an edge between two vertices.
     *
     * @return std::vector<std::vector<int>> A vector of pairs representing the graph edges.
     */
    std::pair<Edges, Edges> getGraphEdges();

    std::filesystem::path inputFilePath;
    std::filesystem::path outputFilePath;

    util::GridDimensions gridDimensions;
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
    std::vector<int> reverseOffsets;
    std::vector<int> reverseNeighbors;
    std::priority_queue<int> degreeQueue;
    std::vector<util::Position> vertexPositions;
    std::vector<util::Position> paddedPositions;
    std::set<util::Position> occupiedCells;
    std::set<util::Position> paddedCells;
};

#endif // GRAPH_HPP
