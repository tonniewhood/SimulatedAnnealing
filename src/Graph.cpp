
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "Graph.hpp"

// I'd prefer to not do any I/O in the constructor. That'll be the "readInputFile method
Graph::Graph(const std::filesystem::path& inputFilePath, const std::filesystem::path& outputFilePath)
    : inputFilePath(inputFilePath)
    , outputFilePath(outputFilePath)
    , gridDimensions(-1, -1)
    , numVertices(-1)
    , offsets(std::vector<int>())
    , neighbors(std::vector<int>())
    , reverseOffsets(std::vector<int>())
    , reverseNeighbors(std::vector<int>())
    , degreeQueue(std::priority_queue<VertexDegree>())
    , vertexPositions(std::vector<util::Position>())
    , paddedPositions(std::vector<util::Position>())
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
    auto [forwardEdges, reverseEdges] = this->getGraphEdges();

    // Reserve the offsets and neighbors vectors based on the number of vertices
    this->offsets.resize(this->numVertices + 1, 0); // +1 allows for the end point of the last vertex
    this->neighbors.reserve(forwardEdges.size()); // Reserve space for each edge
    this->reverseOffsets.resize(this->numVertices + 1, 0);
    this->reverseNeighbors.reserve(reverseEdges.size());

    // I think there's got to be a better way to unroll this edge vector, but for now we'll do it
    // naively
    for (int vertex = 0; vertex < this->numVertices; ++vertex) {
        // Set the offset for this vertex to the current size of the neighbors vector
        this->offsets[vertex + 1] = static_cast<int>(forwardEdges[vertex].size() + this->offsets[vertex]);
        this->reverseOffsets[vertex + 1] = static_cast<int>(reverseEdges[vertex].size() + this->reverseOffsets[vertex]);

        int vertexDegree = static_cast<int>(forwardEdges[vertex].size() + reverseEdges[vertex].size());
        this->degreeQueue.push(VertexDegree(vertex, vertexDegree));

        // Add all neighbors of this vertex to the neighbors vector
        for (int neighbor : forwardEdges[vertex]) {
            this->neighbors.push_back(neighbor);
        }

        for (int neighbor : reverseEdges[vertex]) {
            this->reverseNeighbors.push_back(neighbor);
        }
    }

    // Final validation to ensure the sizes match
    if (static_cast<int>(this->neighbors.size()) != this->offsets.back()) {
        std::cerr << "Error: Mismatch in neighbors size and offsets last value." << std::endl;
        exit(EXIT_FAILURE);
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
                      << ") does not match number of vertices (" << this->numVertices << ")." << std::endl;
            return false;
        }

        // Check for duplicate positions
        std::unordered_map<std::string, bool> positionMap;
        for (const auto& pos : positions) {
            if (pos.col < 0 || pos.col >= this->gridDimensions.width || pos.row < 0
                || pos.row >= this->gridDimensions.height) {
                std::cerr << "Error: Position (" << pos.col << ", " << pos.row << ") is out of grid bounds."
                          << std::endl;
                return false;
            }

            std::string key = std::to_string(pos.row) + "," + std::to_string(pos.col);
            if (positionMap.find(key) != positionMap.end()) {
                std::cerr << "Error: Duplicate position found: (" << pos.row << ", " << pos.col << ")." << std::endl;
                return false;
            }
            positionMap[key] = true;
        }

        this->vertexPositions = positions;
        for (const auto& pos : positions) {
            this->occupiedCells.insert(pos);
        }

    } else {
        // If no positions are provided, place vertices in order on the grid
        this->vertexPositions.clear();
        this->vertexPositions.reserve(this->numVertices);
        for (int i = 0; i < this->numVertices; ++i) {
            int col = i % this->gridDimensions.width;
            int row = i / this->gridDimensions.width;
            if (row >= this->gridDimensions.height) {
                std::cerr << "Error: Not enough space on the grid to place all vertices." << std::endl;
                return false;
            }
            this->vertexPositions.emplace_back(row, col);
            this->occupiedCells.insert(util::Position(row, col));
        }
    }

    this->paddedCells.clear();
    this->paddedPositions.clear();
    this->paddedPositions.reserve(this->numVertices * 4); // Worst case, every vertex has all 4 pads
    // Identify padded positions
    for (const auto& pos : this->occupiedCells) {
        // Check all 4 possible adjacent positions (up, down, left, right)
        std::vector<util::Position> adjacentPositions = {
            util::Position(pos.row - 1, pos.col), // Up
            util::Position(pos.row + 1, pos.col), // Down
            util::Position(pos.row, pos.col - 1), // Left
            util::Position(pos.row, pos.col + 1) // Right
        };

        for (const auto& adj : adjacentPositions) {
            // Check if the adjacent position is within grid bounds and not occupied
            bool cellInBounds = adj.row >= 0 && adj.row < this->gridDimensions.height && adj.col >= 0
                && adj.col < this->gridDimensions.width;
            bool cellOccupied = this->occupiedCells.find(adj) != this->occupiedCells.end();
            bool cellAlreadyPadded = this->paddedCells.find(adj) != this->paddedCells.end();
            if (cellInBounds && !cellOccupied && !cellAlreadyPadded) {
                this->paddedCells.insert(adj);
                this->paddedPositions.push_back(adj);
            }
        }
    }

    this->paddedPositions.shrink_to_fit();

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
 * @brief Retrieves the reverse neighbors of a given vertex.
 * @param vertex The vertex for which to retrieve reverse neighbors.
 * @return A vector of reverse neighboring vertices.
 */
std::vector<int> Graph::getVertexReverseNeighbors(int vertex) const
{
    if (vertex < 0 || vertex >= this->numVertices) {
        std::cerr << "Error: Vertex index out of bounds." << std::endl;
        return {}; // Return an empty vector
    }

    int start = this->reverseOffsets[vertex];
    int end = this->reverseOffsets[vertex + 1];
    return std::vector<int>(this->reverseNeighbors.begin() + start, this->reverseNeighbors.begin() + end);
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
 * @brief Retrieves the vertex position if a given position is occupied, or -1 if not
 * @param pos The position to check for a vertex
 * @return The vertex index at that position, or -1 if unoccupied
 */
int Graph::getVertexAtPosition(const util::Position& pos) const
{
    auto iter = std::find(this->vertexPositions.begin(), this->vertexPositions.end(), pos);
    if (iter != this->vertexPositions.end()) {
        return static_cast<int>(std::distance(this->vertexPositions.begin(), iter));
    }
    return -1; // Return -1 if the position is unoccupied
}

/**
 * @brief Retrieves the valid, unoccupied neighboring cells of a given position.
 * @param pos The position for which to retrieve neighboring cells.
 * @return A vector of valid, unoccupied neighboring positions.
 */
std::vector<util::Position> Graph::getValidAdjacentCells(
    const util::Position& pos, std::unordered_set<util::Position>& visitedPositions)
{
    std::vector<util::Position> adjCells;
    adjCells.reserve(4); // A position can have at most 4 adjacent cells
    int height = this->gridDimensions.height;
    int width = this->gridDimensions.width;

    if (pos.row > 0 && visitedPositions.find(util::Position(pos.row - 1, pos.col)) == visitedPositions.end()) {
        adjCells.push_back(util::Position(pos.row - 1, pos.col));
        visitedPositions.insert(util::Position(pos.row - 1, pos.col));
    }
    if (pos.row < height - 1 && visitedPositions.find(util::Position(pos.row + 1, pos.col)) == visitedPositions.end()) {
        adjCells.push_back(util::Position(pos.row + 1, pos.col));
        visitedPositions.insert(util::Position(pos.row + 1, pos.col));
    }
    if (pos.col > 0 && visitedPositions.find(util::Position(pos.row, pos.col - 1)) == visitedPositions.end()) {
        adjCells.push_back(util::Position(pos.row, pos.col - 1));
        visitedPositions.insert(util::Position(pos.row, pos.col - 1));
    }
    if (pos.col < width - 1 && visitedPositions.find(util::Position(pos.row, pos.col + 1)) == visitedPositions.end()) {
        adjCells.push_back(util::Position(pos.row, pos.col + 1));
        visitedPositions.insert(util::Position(pos.row, pos.col + 1));
    }

    adjCells.shrink_to_fit();
    return adjCells;
}

/**
 * @brief Retrieves the furthest neighbor of a given vertex.
 * @param vertex The vertex for which to retrieve the furthest neighbor.
 * @return A pair containing the furthest neighbor vertex and its position.
 */
std::pair<int, util::Position> Graph::getFurthestNeighbor(int vertex) const
{
    if (vertex < 0 || vertex >= this->numVertices) {
        std::cerr << "Error: Vertex index out of bounds." << std::endl;
        return { -1, util::Position(-1, -1) }; // Return an invalid pair
    }

    util::Position vertexPos = this->getVertexPosition(vertex);
    if (vertexPos.row == -1 || vertexPos.col == -1) {
        std::cerr << "Error: Vertex position is invalid." << std::endl;
        return { -1, util::Position(-1, -1) }; // Return an invalid pair
    }

    std::vector<int> neighbors = this->getVertexNeighbors(vertex);
    std::vector<int> reverseNeighbors = this->getVertexReverseNeighbors(vertex);
    if (reverseNeighbors.empty() && neighbors.empty()) {
        std::cerr << "Error: Vertex has no adjacent neighbors." << std::endl;
        return { -1, util::Position(-1, -1) }; // Return an invalid pair
    }

    std::vector<int> allAdjacent;
    allAdjacent.reserve(neighbors.size() + reverseNeighbors.size());
    allAdjacent.insert(allAdjacent.end(), neighbors.begin(), neighbors.end());
    allAdjacent.insert(allAdjacent.end(), reverseNeighbors.begin(), reverseNeighbors.end());

    int maxDistance = -1;
    int furthestNeighbor = -1;
    util::Position furthestPos(-1, -1);
    for (const int& neighbor : allAdjacent) {
        util::Position neighborPos = this->getVertexPosition(neighbor);
        if (neighborPos.row == -1 && neighborPos.col == -1) {
            std::cerr << "Warning: Neighbor vertex position is invalid, skipping." << std::endl;
            continue; // Skip invalid positions
        }

        int distance = vertexPos.distanceTo(neighborPos);
        if (distance > maxDistance) {
            maxDistance = distance;
            furthestNeighbor = neighbor;
            furthestPos = neighborPos;
        }
    }

    return { furthestNeighbor, furthestPos };
}

std::pair<int, util::Position> Graph::getCentroidPosition(int vertex, int rowPushDir, int colPushDir) const
{
    auto neighbors = this->getVertexNeighbors(vertex);
    auto reverseNeighbors = this->getVertexReverseNeighbors(vertex);
    if (neighbors.empty() && reverseNeighbors.empty()) {
        return { -1, util::Position(-1, -1) }; // Return an invalid position if there are no neighbors
    }
    std::vector<int> allAdjacent;
    allAdjacent.reserve(neighbors.size() + reverseNeighbors.size());
    allAdjacent.insert(allAdjacent.end(), neighbors.begin(), neighbors.end());
    std::vector<util::Position> neighborPositions;
    neighborPositions.reserve(allAdjacent.size());
    for (const int& neighbor : allAdjacent) {
        util::Position neighborPos = this->getVertexPosition(neighbor);
        if (neighborPos.row == -1 && neighborPos.col == -1) {
            std::cerr << "Warning: Neighbor vertex position is invalid, skipping." << std::endl;
            continue; // Skip invalid positions
        }
        neighborPositions.push_back(neighborPos);
    }

    auto centroid = std::accumulate(neighborPositions.begin(), neighborPositions.end(), util::Position(0, 0),
                        [](util::Position& sum, const util::Position& pos) { return sum + pos; })
        / static_cast<int>(neighborPositions.size());

    centroid.row = std::clamp(centroid.row + rowPushDir, 0, this->gridDimensions.height - 1);
    centroid.col = std::clamp(centroid.col + colPushDir, 0, this->gridDimensions.width - 1);

    int occupyingVertex = this->getVertexAtPosition(centroid);
    return { occupyingVertex, centroid };
}

/**
 * @brief Determines if the given position is occupied by any vertex.
 * @param pos The position to check.
 * @return true if the position is occupied, false otherwise.
 */
bool Graph::isPositionOccupied(const util::Position& pos) const
{
    return this->occupiedCells.find(pos) != this->occupiedCells.end();
}

static inline bool in_bounds(const util::Position& pos, const util::GridDimensions& dims)
{
    return pos.row >= 0 && pos.row < dims.height && pos.col >= 0 && pos.col < dims.width;
}

static inline void erase_all(std::vector<util::Position>& vec, const util::Position& value)
{
    vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
}

static inline void safe_push_back(std::vector<util::Position>& vec, const util::Position& value)
{
    auto iter = std::find(vec.begin(), vec.end(), value);
    if (iter == vec.end()) {
        vec.push_back(value);
    }
}

/**
 * @brief Updates the padded pixels after a swap has been made.
 * @param vacatedPos The position that was vacated by the swap
 * @param filledPos The position that was filled after the swap
 */
void Graph::updatePadding(util::Position vacatedPos, util::Position filledPos)
{
    auto adjacentCells = [](const util::Position& pos) {
        return std::vector<util::Position> { util::Position(pos.row - 1, pos.col), util::Position(pos.row + 1, pos.col),
            util::Position(pos.row, pos.col - 1), util::Position(pos.row, pos.col + 1) };
    };

    // Remove the old source from the occupied cells and add the new one
    this->occupiedCells.erase(vacatedPos);
    this->occupiedCells.insert(filledPos);

    // Remove the filled position from the padded cells
    this->paddedCells.erase(filledPos);
    erase_all(this->paddedPositions, filledPos);

    // Ensure all adjacent positions by the old position have the right status
    bool hasOccupiedNeighbor = false;
    for (const auto& pos : adjacentCells(vacatedPos)) {

        // Verify the position is within bounds and is padded
        if (!in_bounds(pos, this->gridDimensions))
            continue;

        hasOccupiedNeighbor = hasOccupiedNeighbor || this->occupiedCells.count(pos);
        if (!this->paddedCells.count(pos) || this->occupiedCells.count(pos))
            continue;

        bool neighborHasOccupiedNeighbor = false;
        for (const auto& adj : adjacentCells(pos)) {
            if (!in_bounds(adj, this->gridDimensions))
                continue;
            if (this->occupiedCells.count(adj)) {
                neighborHasOccupiedNeighbor = true;
                break;
            }
        }

        if (!neighborHasOccupiedNeighbor) {
            // If none of the neighbors are occupied, this position should be removed from padding
            this->paddedCells.erase(pos);
            erase_all(this->paddedPositions, pos);
        }
    }

    if (hasOccupiedNeighbor) {
        // If the vacated position has any occupied neighbors, it should be padded
        this->paddedCells.insert(vacatedPos);
        safe_push_back(this->paddedPositions, vacatedPos);
    } else {
        // If the vacated position has no occupied neighbors, it should not be padded
        this->paddedCells.erase(vacatedPos);
        erase_all(this->paddedPositions, vacatedPos);
    }

    // Now, add the cells surrounding the new filled position to the padded cells if they're not
    // occupied or already there
    for (const auto& pos : adjacentCells(filledPos)) {
        // If the position is known to be occupied, just move on
        if (this->occupiedCells.find(pos) != this->occupiedCells.end())
            continue;

        // If the position is already padded, just move on
        if (this->paddedCells.find(pos) != this->paddedCells.end())
            continue;

        // Check if the position is within grid bounds
        bool cellInBounds = pos.row >= 0 && pos.row < this->gridDimensions.height && pos.col >= 0
            && pos.col < this->gridDimensions.width;
        if (!cellInBounds)
            continue;

        this->paddedCells.insert(pos);
        safe_push_back(this->paddedPositions, pos);
    }
}

/**
 * @brief Updates placement of the src and dst vertices in the graph
 * @param src The source vertex index
 * @param dst The destination vertex index. If -1, it means we filled an empty space
 * @param srcPos The new position for the source vertex
 * @param dstPos The new position for the destination vertex.
 */
void Graph::updateVertexPositions(int src, int dst, const util::Position& srcPos, const util::Position& dstPos)
{
    if (src < 0 || src >= this->numVertices) {
        std::cerr << "Error: Source vertex index out of bounds." << std::endl;
        return;
    }
    if (dst != -1 && (dst < 0 || dst >= this->numVertices)) {
        std::cerr << "Error: Destination vertex index out of bounds." << std::endl;
        return;
    }
    if (!in_bounds(srcPos, this->gridDimensions)) {
        std::cerr << "Error: Source position out of grid bounds." << std::endl;
        return;
    }
    if (!in_bounds(dstPos, this->gridDimensions)) {
        std::cerr << "Error: Destination position out of grid bounds." << std::endl;
        return;
    }

    if (dst == -1) {
        // We filled an empty space, so just update the source vertex position and remove the source position from the
        // occupied cells
        this->vertexPositions[src] = dstPos;
        this->occupiedCells.erase(srcPos);
        this->occupiedCells.insert(dstPos);
    } else {
        // We swapped two vertices, so update both positions
        std::swap(this->vertexPositions[src], this->vertexPositions[dst]);
    }
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
bool Graph::reportResults() const
{
    std::ofstream outputFile(this->outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file: " << this->outputFilePath << std::endl;
        return false;
    }

    // Write the grid dimensions and number of vertices
    outputFile << "For grid dimensions " << this->gridDimensions.width << " x " << this->gridDimensions.height
               << " with " << this->numVertices << " vertices:" << std::endl;
    outputFile << "Graph layout:" << std::endl;
    outputFile << "Final score: " << this->scoreGraphLayout() << std::endl;
    outputFile << "[positions in (row, col) format]" << std::endl;
    outputFile << "===========================" << std::endl;
    // List the positions of each vertex
    for (int vertex = 0; vertex < this->numVertices; ++vertex) {
        util::Position pos = this->getVertexPosition(vertex);
        outputFile << "Node " << vertex << " placed at (" << pos.row << ", " << pos.col << ")" << std::endl;
    }
    // List the edges of the graph and their distances
    outputFile << "---------------------------" << std::endl;
    for (int vertex = 0; vertex < this->numVertices; ++vertex) {
        std::vector<int> neighbors = this->getVertexNeighbors(vertex);
        util::Position vertexPos = this->getVertexPosition(vertex);
        for (int neighbor : neighbors) {
            util::Position neighborPos = this->getVertexPosition(neighbor);
            int distance = vertexPos.distanceTo(neighborPos);
            outputFile << "Edge (" << vertex << " -> " << neighbor << ") with distance " << distance << std::endl;
        }
    }
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
void Graph::validateHeaderInfo(std::ifstream& inputFile, std::string& line, std::stringstream& lineStream)
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
        std::cerr << "Error: Expected file to begin with grid dimentions ('g'), but found '" << lineType << "' instead."
                  << std::endl;
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
        std::cerr << "Error: Expected second line to specify number of vertices ('v'), but found '" << lineType
                  << "' instead." << std::endl;
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
void Graph::validateEdgeInfo(std::ifstream& inputFile, std::string& line, std::stringstream& lineStream,
    Graph::Edges& forwardEdges, Graph::Edges& reverseEdges, int& maxVertexIndex)
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
            std::cerr << "Warning: Expected edge definition ('e'), but found '" << lineType << "'. Skipping line."
                      << std::endl;
            lineStream.clear();
            lineType = '\0';
            value1 = value2 = 0;
            continue;
        }

        // Validate edge values
        if (value1 < 0 || value1 >= this->numVertices || value2 < 0 || value2 >= this->numVertices) {
            std::cerr << "Warning: Edge values must be between 0 and " << (this->numVertices - 1) << ". Skipping edge ("
                      << value1 << ", " << value2 << ")." << std::endl;
            lineStream.clear();
            lineType = '\0';
            value1 = value2 = 0;
            continue;
        }
        if (value1 == value2) {
            std::cerr << "Warning: Self-loops are not allowed. Skipping edge (" << value1 << ", " << value2 << ")."
                      << std::endl;
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
        while (value1 >= static_cast<int>(forwardEdges.size())) {
            forwardEdges.resize(forwardEdges.size() + 50, std::vector<int>()); // Expand by 50 vertices at a time
        }

        while (value2 >= static_cast<int>(reverseEdges.size())) {
            reverseEdges.resize(reverseEdges.size() + 50, std::vector<int>()); // Expand by 50 vertices at a time
        }

        // Add the edge to the list
        forwardEdges[value1].push_back(value2);
        reverseEdges[value2].push_back(value1);

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
std::pair<Graph::Edges, Graph::Edges> Graph::getGraphEdges()
{
    // Get a stream from the input file
    std::ifstream inputFile(this->inputFilePath);
    Graph::Edges forwardEdges;
    Graph::Edges reverseEdges;

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
    forwardEdges.resize(100, std::vector<int>());
    reverseEdges.resize(100, std::vector<int>());

    this->validateEdgeInfo(inputFile, line, lineStream, forwardEdges, reverseEdges, maxVertexIndex);

    // Final validation to ensure the highest vertex index matches the specified number of vertices
    if (maxVertexIndex != this->numVertices - 1) {
        std::cerr << "Error: The highest vertex index found (" << maxVertexIndex
                  << ") does not match the specified number of vertices (" << this->numVertices << ")." << std::endl;
        exit(EXIT_FAILURE);
    }

    forwardEdges.resize(this->numVertices); // Resize to the actual number of vertices
    reverseEdges.resize(this->numVertices);
    inputFile.close();
    return std::make_pair(forwardEdges, reverseEdges);
}

/**
 * @brief Resets the graph to its initial state, clearing all vertex positions and padded cells.
 */
void Graph::resetGraph()
{
    this->vertexPositions.clear();
    this->occupiedCells.clear();
    this->paddedCells.clear();
    this->paddedPositions.clear();
}
