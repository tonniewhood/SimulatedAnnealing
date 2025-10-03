
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "Graph.hpp"

// I'd prefer to not do any I/O in the constructor. That'll be the "readInputFile method
Graph::Graph(
    const std::filesystem::path& inputFilePath, const std::filesystem::path& outputFilePath)
    : inputFilePath(inputFilePath)
    , outputFilePath(outputFilePath)
    , gridDimensions(-1, -1)
    , numVertices(-1)
    , offsets(std::vector<int>())
    , neighbors(std::vector<int>())
    , vertexPositions(std::vector<util::Position>())

{
}

Graph::~Graph()
{
    // Currently empty destructor
}

/**
 * @brief Reads the input file specified by inputFilePath and processes its contents.
 */
void Graph::readInputFile()
{
    auto edges = this->getGraphEdges();

    // Reserve the offsets and neighbors vectors based on the number of vertices
    this->offsets.resize(
        this->numVertices + 1, 0); // +1 allows for the end point of the last vertex
    this->neighbors.reserve(edges.size()); // Reserve space for each edge

    // I think there's got to be a better way to unroll this edge vector, but for now we'll do it
    // naively
    for (int vertex = 0; vertex < this->numVertices; ++vertex) {
        // Set the offset for this vertex to the current size of the neighbors vector
        this->offsets[vertex + 1] = static_cast<int>(edges[vertex].size() + this->offsets[vertex]);

        // Add all neighbors of this vertex to the neighbors vector
        for (int neighbor : edges[vertex]) {
            this->neighbors.push_back(neighbor);
        }
    }
}

/**
 * @brief Initializes vertex positions. If no positions are provided, vertices are placed in
 * order on a grid of size gridWidth x gridHeight.
 * @param positions Optional vector of positions to initialize the vertices. Repeated positions
 * are not allowed.
 * @return true if initialization is successful, false otherwise.
 */
bool Graph::initializeVertexPositions(const std::vector<util::Position>& positions)
{
    if (!positions.empty()) {
        // If positions are provided, validate them
        if (static_cast<int>(positions.size()) != this->numVertices) {
            std::cerr << "Error: Number of provided positions (" << positions.size()
                      << ") does not match number of vertices (" << this->numVertices << ")."
                      << std::endl;
            return false;
        }

        // Check for duplicate positions
        std::unordered_map<std::string, bool> positionMap;
        for (const auto& pos : positions) {
            if (pos.col < 0 || pos.col >= this->gridDimensions.width || pos.row < 0
                || pos.row >= this->gridDimensions.height) {
                std::cerr << "Error: Position (" << pos.col << ", " << pos.row
                          << ") is out of grid bounds." << std::endl;
                return false;
            }

            std::string key = std::to_string(pos.row) + "," + std::to_string(pos.col);
            if (positionMap.find(key) != positionMap.end()) {
                std::cerr << "Error: Duplicate position found: (" << pos.row << ", " << pos.col
                          << ")." << std::endl;
                return false;
            }
            positionMap[key] = true;
        }

        this->vertexPositions = positions;
    } else {
        // If no positions are provided, place vertices in order on the grid
        this->vertexPositions.clear();
        for (int i = 0; i < this->numVertices; ++i) {
            int col = i % this->gridDimensions.width;
            int row = i / this->gridDimensions.width;
            if (row >= this->gridDimensions.height) {
                std::cerr << "Error: Not enough space on the grid to place all vertices."
                          << std::endl;
                return false;
            }
            this->vertexPositions.emplace_back(row, col);
        }
    }

    return true;
}

/**
 * @brief Retrieves the neighbors of a given vertex.
 * @param vertex The vertex for which to retrieve neighbors.
 * @return A vector of neighboring vertices.
 */
std::vector<int> Graph::getVertexNeighbors(int vertex) const
{
    if (vertex < 0 || vertex >= this->numVertices) {
        std::cerr << "Error: Vertex index out of bounds." << std::endl;
        return {}; // Return an empty vector
    }

    int start = this->offsets[vertex];
    int end = this->offsets[vertex + 1];

    return std::vector<int>(this->neighbors.begin() + start, this->neighbors.begin() + end);
}

/**
 * @brief Retrieves the position of a given vertex.
 * @param vertex The vertex for which to retrieve the position.
 * @return The position of the vertex.
 */
util::Position Graph::getVertexPosition(int vertex) const
{
    if (vertex < 0 || vertex >= this->numVertices) {
        std::cerr << "Error: Vertex index out of bounds." << std::endl;
        return util::Position(-1, -1); // Return an invalid position
    }

    if (this->vertexPositions.empty()) {
        std::cerr << "Error: Vertex positions have not been initialized." << std::endl;
        return util::Position(-1, -1); // Return an invalid position
    }

    if (vertex >= static_cast<int>(this->vertexPositions.size())) {
        std::cerr << "Error: Vertex index exceeds initialized positions." << std::endl;
        return util::Position(-1, -1); // Return an invalid position
    }

    return this->vertexPositions[vertex];
}

/**
 * @brief Scores the current layout of the graph based on the sum of squared
 * distances between connected vertices.
 * @return The score of the current graph layout.
 */
int Graph::scoreGraphLayout() const
{
    int score = 0;
    for (int vertex = 0; vertex < this->getNumVertices(); vertex++) {

        // Start by grabbing all the neighbors of this vertex and its position
        std::vector<int> neighbors = this->getVertexNeighbors(vertex);
        util::Position vertexPos = this->getVertexPosition(vertex);
        // Now, for each neighbor, calculate the squared distance and add it to the score
        for (int neighbor : neighbors) {

            util::Position neighborPos = this->getVertexPosition(neighbor);
            int distance = vertexPos.distanceTo(neighborPos);
            score += distance * distance; // Square the distance
        }
    }
    return score;
}

/**
 * @brief Reports the results of the graph layout based on input flags
 * @param flags A vector of strings representing various flags that determine what additional
 * information to include in the report.
 * @return bool indicating success or failure of the report operation.
 */
bool Graph::reportResults(const std::vector<std::string>& flags) const
{
    std::ofstream outputFile(this->outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file: " << this->outputFilePath << std::endl;
        return false;
    }

    // Write the grid dimensions and number of vertices
    outputFile << "For grid dimensions " << this->gridDimensions.width << " x "
               << this->gridDimensions.height << " with " << this->numVertices
               << " vertices:" << std::endl;
    outputFile << "Graph layout:" << std::endl;
    outputFile << "Final score: " << this->scoreGraphLayout() << std::endl;
    outputFile << "[positions in (row, col) format]" << std::endl;
    outputFile << "===========================" << std::endl;
    // List the positions of each vertex
    for (int vertex = 0; vertex < this->numVertices; ++vertex) {
        util::Position pos = this->getVertexPosition(vertex);
        outputFile << "Node " << vertex << " placed at (" << pos.row << ", " << pos.col << ")"
                   << std::endl;
    }
    // List the edges of the graph and their distances
    outputFile << "---------------------------" << std::endl;
    for (int vertex = 0; vertex < this->numVertices; ++vertex) {
        std::vector<int> neighbors = this->getVertexNeighbors(vertex);
        util::Position vertexPos = this->getVertexPosition(vertex);
        for (int neighbor : neighbors) {
            util::Position neighborPos = this->getVertexPosition(neighbor);
            int distance = vertexPos.distanceTo(neighborPos);
            outputFile << "Edge (" << vertex << " -> " << neighbor << ") with distance " << distance
                       << std::endl;
        }
    }
    std::cout << "Wrote Results to '" << this->outputFilePath << "'" << std::endl;
    outputFile.close();
    return true;
}

/**
 * @brief Validates and extracts the header information from the input file.
 *
 * @param inputFile The input file stream.
 * @param line A string to hold the current line being processed.
 * @param lineStream A stringstream to parse the current line.
 */
void Graph::validateHeaderInfo(
    std::ifstream& inputFile, std::string& line, std::stringstream& lineStream)
{
    char lineType = '\0';
    int value1 = 0, value2 = 0;
    // First line should be the grid dimensions
    do {
        std::getline(inputFile, line);
    } while (line.find_first_not_of(" \t\n\r") == std::string::npos); // Skip empty lines
    lineStream.str(line);
    lineStream >> lineType >> value1 >> value2;
    lineStream.clear();
    if (lineType != 'g') {
        std::cerr << "Error: Expected file to begin with grid dimentions ('g'), but found '"
                  << lineType << "' instead." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Validate grid dimensions and store them
    if (value1 <= 0 || value2 <= 0) {
        std::cerr << "Error: Grid dimensions must be positive integers." << std::endl;
        exit(EXIT_FAILURE);
    }
    this->gridDimensions = util::GridDimensions(value1, value2);

    // Next line should be the number of vertices
    do {
        std::getline(inputFile, line);
    } while (line.find_first_not_of(" \t\n\r") == std::string::npos); // Skip empty lines
    lineStream.str(line);
    lineStream >> lineType >> value1;
    lineStream.clear();
    if (lineType != 'v') {
        std::cerr << "Error: Expected second line to specify number of vertices ('v'), but found '"
                  << lineType << "' instead." << std::endl;
        exit(EXIT_FAILURE);
    }

    // Validate number of vertices and store it
    if (value1 <= 0) {
        std::cerr << "Error: Number of vertices must be a positive integer." << std::endl;
        exit(EXIT_FAILURE);
    }
    this->numVertices = value1;
}

/**
 * @brief Validates and extracts edge information from the input file.
 *
 * @param inputFile The input file stream.
 * @param line A string to hold the current line being processed.
 * @param lineStream A stringstream to parse the current line.
 * @param edges A vector of vectors to store the edges.
 * @param maxVertexIndex An integer to track the maximum vertex index seen.
 */
void Graph::validateEdgeInfo(std::ifstream& inputFile, std::string& line,
    std::stringstream& lineStream, std::vector<std::vector<int>>& edges, int& maxVertexIndex)
{
    char lineType = '\0';
    int value1 = 0, value2 = 0;
    while (std::getline(inputFile, line)) {

        if (line.find_first_not_of(" \t\n\r") == std::string::npos)
            continue; // Skip empty lines

        lineStream.str(line);
        lineStream >> lineType >> value1 >> value2;

        // Validate line type, and skip if it's not an edge definition
        // Should be 'e' for edge
        if (lineType != 'e') {
            std::cerr << "Warning: Expected edge definition ('e'), but found '" << lineType
                      << "'. Skipping line." << std::endl;
            lineStream.clear();
            lineType = '\0';
            value1 = value2 = 0;
            continue;
        }

        // Validate edge values
        if (value1 < 0 || value1 >= this->numVertices || value2 < 0
            || value2 >= this->numVertices) {
            std::cerr << "Warning: Edge values must be between 0 and " << (this->numVertices - 1)
                      << ". Skipping edge (" << value1 << ", " << value2 << ")." << std::endl;
            lineStream.clear();
            lineType = '\0';
            value1 = value2 = 0;
            continue;
        }
        if (value1 == value2) {
            std::cerr << "Warning: Self-loops are not allowed. Skipping edge (" << value1 << ", "
                      << value2 << ")." << std::endl;
            lineStream.clear();
            lineType = '\0';
            value1 = value2 = 0;
            continue;
        }

        // Track the maximum vertex index seen so far for resizing and error checking later
        if (value1 > maxVertexIndex || value2 > maxVertexIndex)
            maxVertexIndex = std::max(value1, value2);

        // Make sure we have space for this edge by checking the first value against the current
        // size
        while (value1 >= static_cast<int>(edges.size())) {
            edges.resize(edges.size() + 50, std::vector<int>()); // Expand by 50 vertices at a time
        }

        // Add the edge to the list
        edges[value1].push_back(value2);

        lineStream.clear();
        lineType = '\0';
        value1 = value2 = 0;
    }
}

/**
 * @brief Reads the graph edges from the input file and returns them as a vector of pairs. Each pair
 * represents an edge between two vertices.
 *
 * @return std::vector<std::vector<int>> A vector of pairs representing the graph edges.
 */
std::vector<std::vector<int>> Graph::getGraphEdges()
{
    // Get a stream from the input file
    std::ifstream inputFile(this->inputFilePath);
    std::vector<std::vector<int>> edges;

    // If the file can't be opened, we're kind of stuck, so we just exit. There shouldn't be any
    // streams open, or allocated memory at this point, so there shouldn't be a need to clean up
    // at all.
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open input file: " << this->inputFilePath << std::endl;
        exit(EXIT_FAILURE);
    }

    // Get the necessary variables ready to parse the file
    std::string line;
    std::stringstream lineStream;

    this->validateHeaderInfo(inputFile, line, lineStream);

    int maxVertexIndex = 0;
    edges.resize(
        100, std::vector<int>()); // Resize to 100 vertices, initializing each with an empty vector

    this->validateEdgeInfo(inputFile, line, lineStream, edges, maxVertexIndex);

    // Final validation to ensure the highest vertex index matches the specified number of vertices
    if (maxVertexIndex != this->numVertices - 1) {
        std::cerr << "Error: The highest vertex index found (" << maxVertexIndex
                  << ") does not match the specified number of vertices (" << this->numVertices
                  << ")." << std::endl;
        exit(EXIT_FAILURE);
    }

    edges.resize(this->numVertices); // Resize to the actual number of vertices
    inputFile.close();
    return edges;
}
