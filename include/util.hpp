
#ifndef UTIL_HPP
#define UTIL_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace util {

struct Args {
    std::vector<std::string> flags;
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

    int distanceTo(const Position& other) const
    {
        return std::abs(row - other.row) + std::abs(col - other.col);
    }
};

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