#!/usr/bin/env bash

# Lab04 Setup Script
# This script helps set up the development environment for the project

set -e  # Exit on error

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_PATH="$PROJECT_ROOT/venv"
ALT_VENV_PATH="$PROJECT_ROOT/.venv"

echo "=== Lab04 Project Setup ==="
echo "Project root: $PROJECT_ROOT"
echo

# Check for Python
if command -v python3 &> /dev/null; then
    PYTHON_CMD=python3
elif command -v python &> /dev/null && [[ "$($(command -v python) --version 2>&1)" =~ ^Python\ 3\. ]]; then
    PYTHON_CMD=python
else
    echo "Error: Python 3 not found. Consider installing to use visualization features."
    exit 1
fi

echo "Python 3 found: $(python3 --version)"

# Check for virtual environment
if [[ -d "$VENV_PATH" ]] || [[ -d "$ALT_VENV_PATH" ]]; then
    [[ -d "$VENV_PATH" ]] || echo "Virtual environment already exists at $ALT_VENV_PATH"
    [[ -d "$ALT_VENV_PATH" ]] || echo "Virtual environment already exists at $VENV_PATH"
    read -p "Recreate it? [y/N]: " recreate
    if [[ $recreate =~ ^[Yy]$ ]]; then
        echo "Removing existing virtual environment..."
        rm -rf "$VENV_PATH"
        make_venv=1
    else
        echo "Using existing virtual environment."
        make_venv=0
    fi
else 
    echo "No existing virtual environment found. Create one? [y/N]: "
    read -p "" create_venv
    if [[ $create_venv =~ ^[Nn]$ ]]; then
        echo "Skipping virtual environment creation."
        echo "Pip may not be available, thus some features may not work."
    else
        make_venv=1
    fi
fi

# Create virtual environment if needed
if [[ "$make_venv" -eq 1 ]]; then
    echo "Creating Python virtual environment..."
    if $PYTHON_CMD -m venv "$VENV_PATH"; then
    	echo "Virtual environment created"
    elif $PYTHON_CMD -m virtualenv "$VENV_PATH"; then
        echo "Virtual environment created"
    else
	echo "Could not create virtual environment"
	exit 1
    fi
fi

# Activate and install dependencies
echo "Activating virtual environment and installing dependencies..."

if [[ -f "$VENV_PATH/bin/activate" ]]; then
    source "$VENV_PATH/bin/activate"
elif [[ -f "$ALT_VENV_PATH/bin/activate" ]]; then
    source "$ALT_VENV_PATH/bin/activate"
fi

export PATH=/usr/bin:/bin:$PATH
hash -r
export CC=$(which gcc)
export CXX=$(which g++)
export CCACHE_DISABLE=1
export CMAKE_GENERATOR=Ninja          # respected by scikit-build / many CMake-based pip builds
export CMAKE_MAKE_PROGRAM=$(which ninja)
export CMAKE_ARGS="-DCMAKE_C_COMPILER=$(which gcc) -DCMAKE_CXX_COMPILER=$(which g++) -G Ninja -DCMAKE_MAKE_PROGRAM=$(which ninja)"


# expose BSD/POSIX bits and make sure u_int is visible even if headers include order varies
export CFLAGS="-O2 -D__BSD_VISIBLE=1 -D_POSIX_C_SOURCE=200809L -include sys/types.h"
export CXXFLAGS="-O2 -D__BSD_VISIBLE=1 -D_POSIX_C_SOURCE=200809L -include sys/types.h"

# Install required Python packages
pip install --upgrade pip
pip install networkx numpy pyqt5 pyqtgraph scipy imageio pandas

echo
echo "Setup complete. To activate the virtual environment in the future, run:"
echo "source $VENV_PATH/bin/activate"
echo "If you encountered any issues, please refer to the README.md for troubleshooting tips."
