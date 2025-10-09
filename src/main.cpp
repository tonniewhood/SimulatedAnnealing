
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Graph.hpp"
#include "pyViz.hpp"
#include "simulateAnnealing.hpp"
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

#define INITIAL_TEMPERATURE 10000000.0f
#define COOLING_RATE 0.99999f

int main(int argc, char* argv[])
{
    // Parse command line arguments
    util::Args args = util::parseCommandLineArgs(argc, argv);

    std::filesystem::path inputFilePath
        = util::getInputFileLocation(args.positional[0], std::string(PROJECT_ROOT), std::string(DATA_DIR));
    std::filesystem::path outputFilePath = util::createOutputFilePath(inputFilePath, args.positional[1]);

    // If I don't have a valid input file path, I can't really do anything. So we'll have to quit.
    if (inputFilePath.empty()) {
        std::cerr << "Error: Could not resolve input file path." << std::endl;
        std::cerr << "Found: " << inputFilePath << std::endl;
        return 1;
    }

    // Realistically, this should never fire, but I'd rather make sure to have a valid output path
    // before using it.
    if (outputFilePath.empty()) {
        std::cerr << "Error: Could not create output file path. Defaulting to 'output.txt'." << std::endl;
        outputFilePath = std::filesystem::path("output.txt");
    }

    std::cout << "\n=== Lab04 Simulated Annealing ===\n" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Input file:  " << inputFilePath << std::endl;
    std::cout << "  Output file: " << outputFilePath << std::endl;
    std::cout << "  Initial temperature: " << INITIAL_TEMPERATURE << std::endl;
    std::cout << "  Cooling rate: " << COOLING_RATE << std::endl;

    Graph graph(inputFilePath, outputFilePath);
    graph.readInputFile();

    if (args.flagMap.find("--run-analysis") != args.flagMap.end()) {
        std::string runAnalysis = args.flagMap.at("--run-analysis");
        if (util::toLower(runAnalysis) == "true") {
            std::cout << "\n--- Running Annealing Analysis ---" << std::endl;
            std::cout << "Note: This may take several minutes to complete...\n" << std::endl;
            std::vector<double> coolingRates = { 0.9, 0.95, 0.99, 0.995, 0.999, 0.9995, 0.9999, 0.99995, 0.99999 };
            std::filesystem::path analysisDir = inputFilePath.parent_path();
            return sim::runAnnealingAnalysis(graph, INITIAL_TEMPERATURE, coolingRates, 10, analysisDir);
        }
    }

    sim::MutationMethod method = sim::NAIVE;

    if (args.flagMap.find("--mutation-method") != args.flagMap.end()) {
        std::string methodStr = args.flagMap.at("--mutation-method");
        method = sim::stringToMutationMethod(methodStr);
        if (method == sim::UNDEFINED) {
            std::cerr << "Warning: Undefined mutation method '" << methodStr << "'. Defaulting to NAIVE." << std::endl;
            method = sim::NAIVE;
            std::cout << "  Mutation method: NAIVE (fallback)" << std::endl;
        } else {
            std::cout << "  Mutation method: " << methodStr << std::endl;
        }
    } else {
        std::cout << "  Mutation method: NAIVE (default)" << std::endl;
    }

#if HAVE_PYTHON

    util::PlotType plotType = util::NONE;
    if (args.flagMap.find("--plot-type") != args.flagMap.end()) {
        std::string plotTypeStr = args.flagMap.at("--plot-type");
        plotType = util::stringToPlotType(plotTypeStr);
        if (plotType == util::NONE) {
            std::cerr << "Warning: Undefined plot type '" << plotTypeStr << "'. No visualization will be used."
                      << std::endl;
            std::cout << "  Visualization: None" << std::endl;
        } else {
            std::cout << "  Visualization: " << plotTypeStr << std::endl;
        }
    }

    if (plotType & util::ALL_MASK) {
        std::cout << "\n--- Starting Simulation ---" << std::endl;
        std::cout << "Initializing visualization..." << std::endl;

        viz::PyVisualizer viz(graph.getGridWidth(), graph.getGridHeight(), graph.getNumVertices(), graph.getOffsets(),
            graph.getNeighbors(), plotType);

        if (!viz.isInitialized()) {
            std::cerr << "Error: Python visualizer failed to initialize. Running without it." << std::endl;
            sim::simulateAnnealing(graph, INITIAL_TEMPERATURE, COOLING_RATE, method);
        } else if (!viz.isActive()) {
            std::cerr << "Error: Python visualizer is not active. Running without it." << std::endl;
            sim::simulateAnnealing(graph, INITIAL_TEMPERATURE, COOLING_RATE, method);
        } else {

            // Open the visualization windows and then wait for a second to settle the displays
            viz.displayFigures();
            viz.keepAlive(1.0);

            // Start the annealing process in a separate thread
            bool sendUpdates = (plotType & (util::GRID_MASK | util::STATS_MASK)) != 0;
            std::thread annealThread([&graph, &method, &sendUpdates, &viz]() {
                sim::simulateAnnealing(
                    graph, INITIAL_TEMPERATURE, COOLING_RATE, method, sendUpdates, viz.threadControls);
            });

            // Start the visualization loop in the main thread
            viz::visualizationLoop(viz);

            while (!annealThread.joinable())
                ;
            annealThread.join();

            if (!viz.threadControls->stoppedEarly.load()) {
                if (args.flagMap.find("--save-figures") != args.flagMap.end()) {

                    // Get the save figures flag and check if it's true
                    std::string saveFigures = args.flagMap.at("--save-figures");
                    if (util::toLower(saveFigures) == "true") {

                        // If it is, check for the figure-path flag
                        std::string figurePath = "./";
                        if (args.flagMap.find("--figure-path") != args.flagMap.end()) {
                            figurePath = args.flagMap.at("--figure-path");
                        }

                        std::cout << "\n--- Saving Visualization Results ---" << std::endl;
                        std::cout << "Output directory: " << figurePath << std::endl;

                        std::string timeStr = util::getCurrentTimeFormatted();

                        std::string grid_gif
                            = figurePath + sim::mutationMethodToString(method) + "_grid_" + timeStr + ".gif";
                        std::string grid_png
                            = figurePath + sim::mutationMethodToString(method) + "_grid_" + timeStr + ".png";
                        std::string graph_png
                            = figurePath + sim::mutationMethodToString(method) + "_graph_" + timeStr + ".png";

                        std::vector<std::string> stats_pngs = {
                            figurePath + sim::mutationMethodToString(method) + "_Temp_v_Time_" + timeStr + ".png",
                            figurePath + sim::mutationMethodToString(method) + "_Score_v_Time_" + timeStr + ".png",
                            figurePath + sim::mutationMethodToString(method) + "_BestScore_v_Time_" + timeStr + ".png",
                            figurePath + sim::mutationMethodToString(method) + "_ScoreDelta_v_Time_" + timeStr + ".png",
                            figurePath + sim::mutationMethodToString(method) + "_AcceptanceRate_v_Time_" + timeStr
                                + ".png",
                        };

                        viz.saveFigures(grid_gif, grid_png, graph_png, stats_pngs);
                    }
                }
            } else {
                std::cout << "\nProcess terminated early by user - skipping figure save." << std::endl;
            }

            viz.shutdown();
        }
    } else {
        std::cout << "\n--- Starting Simulation ---" << std::endl;
        std::cout << "Running without visualization..." << std::endl;
        sim::simulateAnnealing(graph, INITIAL_TEMPERATURE, COOLING_RATE, method);
    }

#else

    std::cout << "  Visualization: None (compiled without Python support)" << std::endl;
    std::cout << "\n--- Starting Simulation ---" << std::endl;
    std::cout << "Running without visualization..." << std::endl;
    sim::simulateAnnealing(graph, INITIAL_TEMPERATURE, COOLING_RATE, method);

#endif // HAVE_PYTHON

    if (!graph.reportResults()) {
        std::cerr << "Error: Failed to report results." << std::endl;
        return 1;
    }

    std::cout << "Results have been written to: " << outputFilePath << "\n" << std::endl;

    return 0;
}