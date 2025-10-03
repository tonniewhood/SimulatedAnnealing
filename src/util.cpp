
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "util.hpp"

/**
 * @brief Parses command-line arguments into flags and positional arguments. If improper arguments
 * are passed, the program will exit.
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return A struct containing vectors of flags and positional arguments.
 */
util::Args util::parseCommandLineArgs(int argc, char* argv[])
{
    util::Args args;

    auto printUsageAndExit = [&]() {
        std::cerr << "Usage: <input_file_path> <output_file_path> [--flag1 --flag2 ...]"
                  << std::endl; // Update the flags as I get more stuff
        exit(EXIT_FAILURE);
    };

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) { // Argument starts with '--', it's a flag.
            if (i > 3) {
                args.flags.push_back(arg);
            } else {
                std::cerr
                    << "Error: Flags must be provided after the first three positional arguments."
                    << std::endl;
                printUsageAndExit();
            }
        } else { // Positional argument
            if (args.positional.size() > 2 || i > 3) {
                std::cerr << args.positional.size() << std::endl;
                std::cerr << i << std::endl;
                std::cerr << "Error: Only args 1 and 2 may be positional." << std::endl;
                printUsageAndExit();
            }
            args.positional.push_back(arg);
        }
    }

    if (args.positional.size() < 2) {
        std::cerr << "Error: At least two positional arguments are required." << std::endl;
        printUsageAndExit();
    }

    return args;
}

/**
 * @brief Attempts to resolve the input file location by checking the project root and data
 * directory.
 * @param inputFilePath The input file path provided by the user.
 * @param projectRoot The root directory of the project.
 * @param dataDir The data directory where input files are typically stored.
 * @return The resolved input file path if found, otherwise an empty path.
 */
std::filesystem::path util::getInputFileLocation(
    const std::string& inputFileName, const std::string& projectRoot, const std::string& dataDir)
{
    namespace fs = std::filesystem;

    std::cout << "Received input file: " << inputFileName << std::endl;

    fs::path testRootPath = fs::path(projectRoot) / fs::path(inputFileName);
    fs::path testDataPath = fs::path(dataDir) / fs::path(inputFileName);

    if (fs::exists(testRootPath))
        return testRootPath;
    if (fs::exists(testDataPath))
        return testDataPath;
    return fs::path();
}

/**
 * @brief Creates an output file path based on the input file's directory and the provided output
 * file name.
 * @param inputFilePath The path of the input file.
 * @param outputFileName The desired name for the output file.
 * @return The constructed output file path.
 */
std::filesystem::path util::createOutputFilePath(
    const std::filesystem::path& inputFilePath, const std::string& outputFileName)
{
    namespace fs = std::filesystem;

    if (inputFilePath.has_parent_path()) {
        return inputFilePath.parent_path() / fs::path(outputFileName);
    }

    return fs::path(outputFileName);
}
