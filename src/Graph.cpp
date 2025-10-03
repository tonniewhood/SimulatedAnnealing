
#include <fstream>
#include <iostream>
#include <sstream>

/* --- Included in Graph.hpp ---
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
--------------------------------*/

#include "Graph.hpp"

// I'd prefer to not do any I/O in the constructor. That'll be the "readInputFile method
Graph::Graph(
    const std::filesystem::path& inputFilePath, const std::filesystem::path& outputFilePath)
    : inputFilePath(inputFilePath)
    , outputFilePath(outputFilePath)
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

    // Print out the graph
    printf("Offsets:   ");
    for (auto& offset : this->offsets) {
        printf("%2d ", offset);
    }

    printf("\nNeighbors: ");
    for (auto& neighbor : this->neighbors) {
        printf("%2d ", neighbor);
    }
    printf("\n");
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
    this->gridWidth = value1;
    this->gridHeight = value2;

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
    char lineType = '\0';
    int value1 = 0, value2 = 0;

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
