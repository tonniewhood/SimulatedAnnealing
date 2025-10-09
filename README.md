# Simulated Annealing - Lab 04

This project is a basic implementation of simulated annealing for placement of CLBs on an FPGA.

## Overview

In this repo, there's a few things going on that may need explaining. The main body of the project is in C++, and there are no parts of the project that require Python. However, if you want to see a logical implementation of the graph, and the real-time placement of the pieces, then you'll need Python 3. This project utilizes Python 3.12, but that's not a requirement. Additionally, to keep the system clean, all python packages are kept inside of a virtual environment. The Python code is embedded in the C++ application, and runs asynchronously with the C++ side, but does require some thread handling to ensure that data is shared. I preferred this method over writing to a fifo and then picking that up in a Python script or even using a socket.

## Features

- Simulated annealing algorithm
- Embedded Python for visualization and analysis
- OS agnostic build procedures.

## Requirements

- C++ 17
- C++ compiler (g++ used, but clang or LLVM should work fine)
- Make
- Python 3.x (optional)

## Usage

1. Either get the zip file, or clone the repository down
2. Ensure flags in the `makefile` are reasonable for your system/any defaults you'd like to change.
3. Build using make or your chosen compiler directly.
4. Run the program either directly or using the `make run` target.

### Building

Building this project should ideally be relatively simple. There's not many external dependancies to manage, mainly just Python. When using make you have the following targets

* all (run `make` with no arguments)
* run
* clean
* help

Running make should work just fine. However, if there are issues, you can also run the compiler of your choice directly. For clang++ and g++, the command is identical. Simply navigate to the project root, and run the following:

```bash
[g++|clang++] -Iinclude src/Graph.cpp src/main.cpp src/py_viz.cpp [FLAGS] -o [EXE NAME]
```

There does exist potentials for errors, as the project does use the `filesystem` standard library code, which was introduced in C++17. If using linux, use your package manager to get the `build-essential` package. If using Windows, either use something from the [Min-GW project](https://www.mingw-w64.org/) or [Visual Studio](https://visualstudio.microsoft.com/downloads/) based on preference. Either method should provide a compiler that allows for running the specified compilation command.

For this project, the following flags were used to build:

* `-Wall`
* `-Wextra`
* `-g -O0` or `-O3` (dependant on debug or release)
* `-std=c++17`

Additionally, if using MSVC, the command needs to be adjusted. This should be the command:

```bash
cl /std:c++17 /W4 /EHsc /I include src\Graph.cpp src\main.cpp src\py_viz.cpp /Fe:myprog.exe
```

Note though, that I haven't been able to verify on a Windows computer that has MSVC installed, so I can't garauntee that will work.

The preffered version of building though, is using the `makefile`. If using this, just navigate to the project root, and run it. The various targets are explained below

**all**: builds the target executable and all object files associated with it

**run**: builds the executable, and then will run it. The target defaults to using two simple command line arguments, equivilent to running the following:
  * `./target(.exe) input.txt output.txt`

The `run` target does support input arguments to specify what you'd like to run with. These include:

* `ARGS`: command line arguments directly
* `INPUT`: specify the input file while leaving the output to the default
* `OUTPUT`: specify the output file while leaving the input to the default
* `FLAGS`: specify any flags used for analysis

Simply append `[ARGS|INPUT|OUTPUT|FLAGS]="arg1 arg2 ..."` to the end of `make run` to specify your desired make inputs

> NOTE: No flags are currently supported

**clean**: cleans up the build directory

**help**: displays the optionslike a standard `-h` or `--help` command argument

### Running

Running the file is simple, just use either the make command, or run the binary directly. Just ensure that the first two command line aruguments are the input file and the output file (in that order). No additional positional arguments are accepted. Invalid inputs will show a usage message.

> TODO: Include the stuff about analysis here
