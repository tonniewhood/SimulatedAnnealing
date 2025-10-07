
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "util.hpp"

/**
 * @brief Breaks up a flag into the flag name and its value if it has one.
 * @param flag The flag string to be parsed.
 * @return A pair where the first element is the flag name and the second is the flag value (or an
 * empty string if no value is present).
 */
std::pair<std::string, std::string> parseFlag(const std::string& flag)
{
    std::string flagName = flag;
    std::string flagValue;

    // Check if the flag has an associated value
    size_t equalsPos = flag.find('=');
    if (equalsPos != std::string::npos) {
        flagName = flag.substr(0, equalsPos);
        flagValue = flag.substr(equalsPos + 1);
    }

    return { flagName, flagValue };
}

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return A new string with all characters in lowercase.
 */
std::string util::toLower(const std::string& str)
{
    std::string lowerStr = str;
    for (char& c : lowerStr) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return lowerStr;
}

/**
 * @brief Converts a plot type string into the corresponding flag.
 * @param plotTypeStr The input plot type string.
 * @return The corresponding PlotType flag value.
 */
util::PlotType util::stringToPlotType(const std::string& plotTypeStr)
{
    PlotType plotType = 0x00;
    std::string lowerStr = toLower(plotTypeStr);
    std::vector<std::string> types(4, "");
    std::unordered_set<std::string> validType = { "all", "graph", "grid", "stats" };
    std::unordered_set<std::string> seenTypes;
    while (true) {
        size_t commaPos = lowerStr.find(',');
        if (commaPos != std::string::npos) {
            std::string type = lowerStr.substr(0, commaPos);
            if (validType.find(type) == validType.end()) {
                std::cerr << "Warning: Unknown plot type '" << type << "' found."
                          << " Valid types are 'all', 'graph', 'grid', and 'stats'." << std::endl;
                return PlotFlags::NONE; // Return NONE on error
            }

            if (seenTypes.find(type) != seenTypes.end()) {
                std::cerr << "Warning: Duplicate plot type '" << type << "' found." << std::endl;
            } else {
                types.push_back(type);
                seenTypes.insert(type);
            }

            lowerStr = lowerStr.substr(commaPos + 1);
        } else {

            if (validType.find(lowerStr) == validType.end()) {
                std::cerr << "Warning: Unknown plot type '" << lowerStr << "' found."
                          << " Valid types are 'all', 'graph', 'grid', and 'stats'." << std::endl;
                return PlotFlags::NONE; // Return NONE on error
            }

            types.push_back(lowerStr);
            break;
        }
    }

    types.shrink_to_fit();

    for (const std::string& type : types) {
        if (type == "all") {
            plotType |= PlotFlags::ALL_MASK;
        } else if (type == "graph") {
            plotType |= PlotFlags::GRAPH_MASK;
        } else if (type == "grid") {
            plotType |= PlotFlags::GRID_MASK;
        } else if (type == "stats") {
            plotType |= PlotFlags::STATS_MASK;
        }
    }

    return plotType;
}
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
            if (i > 2) {
                args.flagMap.insert(parseFlag(arg));
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
