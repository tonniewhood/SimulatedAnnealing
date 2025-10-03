
#include <iostream>

/* --- Included in Graph.hpp ---
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
*/

/* --- Included in util.hpp ---
#include <filesystem>
#include <string>
#include <vector>
*/

#include "Graph.hpp"
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

int main(int argc, char* argv[])
{
    // Parse command line arguments
    util::Args args = util::parseCommandLineArgs(argc, argv);

    std::cout << "Found flags: ";
    for (const auto& flag : args.flags) {
        std::cout << flag << " ";
    }
    std::cout << std::endl;

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

    return 0;
}