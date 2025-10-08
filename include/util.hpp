
#ifndef UTIL_HPP
#define UTIL_HPP

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace util {

enum PlotFlags {
    NONE = 0x00,
    GRID_MASK = 0x01,
    GRAPH_MASK = 0x02,
    STATS_MASK = 0x04,
    ALL_MASK = 0x07
};
typedef uint8_t PlotType; // Only need 3 bits to determine what to plot

struct Args {
    std::unordered_map<std::string, std::string> flagMap;
    std::vector<std::string> positional;
};

struct GridDimensions {
    int width;
    int height;

    GridDimensions(int width = -1, int height = -1)
        : width(width)
        , height(height)
    {
    }
};

struct Position {
    int row;
    int col;

    Position(int row = -1, int col = -1)
        : row(row)
        , col(col)
    {
    }

    bool operator==(const Position& other) const { return row == other.row && col == other.col; }

    bool operator!=(const Position& other) const { return !(*this == other); }

    bool operator<(const Position& other) const
    {
        if (row != other.row)
            return row < other.row;
        return col < other.col;
    }

    bool operator>(const Position& other) const { return other < *this; }

    std::string toString() const
    {
        return "(" + std::to_string(row) + ", " + std::to_string(col) + ")";
    }

    int distanceTo(const Position& other) const
    {
        return std::abs(row - other.row) + std::abs(col - other.col);
    }
};

template <typename MessageType> struct ThreadControls {
    std::queue<MessageType> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> shouldStop { false };

    ThreadControls() = default;
};

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return A new string with all characters in lowercase.
 */
std::string toLower(const std::string& str);

/**
 * @brief Converts a plot type string into the corresponding enum flag.
 * @param plotTypeStr The input plot type string.
 * @return The corresponding PlotType enum value.
 */
PlotType stringToPlotType(const std::string& plotTypeStr);

/**
 * @brief Parses command-line arguments into flags and positional arguments. If improper arguments
 * are passed, the program will exit.
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return A struct containing vectors of flags and positional arguments.
 */
Args parseCommandLineArgs(int argc, char* argv[]);

/**
 * @brief Attempts to resolve the input file location by checking the project root and data
 * directory.
 * @param inputFilePath The input file path provided by the user.
 * @param projectRoot The root directory of the project.
 * @param dataDir The data directory where input files are typically stored.
 * @return The resolved input file path if found, otherwise an empty path.
 */
std::filesystem::path getInputFileLocation(
    const std::string& inputFilePath, const std::string& projectRoot, const std::string& dataDir);

/**
 * @brief Creates an output file path based on the input file's directory and the provided output
 * file name.
 * @param inputFilePath The path of the input file.
 * @param outputFileName The desired name for the output file.
 * @return The constructed output file path.
 */
std::filesystem::path createOutputFilePath(
    const std::filesystem::path& inputFilePath, const std::string& outputFileName);

};

#endif // UTIL_HPP