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
    $PYTHON_CMD -m venv "$VENV_PATH"
    echo "Virtual environment created"
fi

# Activate and install dependencies
echo "Activating virtual environment and installing dependencies..."

if [[ -f "$VENV_PATH/bin/activate" ]]; then
    source "$VENV_PATH/bin/activate"
elif [[ -f "$ALT_VENV_PATH/bin/activate" ]]; then
    source "$ALT_VENV_PATH/bin/activate"
fi

# Install required Python packages
pip install --upgrade pip
pip install matplotlib networkx numpy

echo
echo 
echo "This project uses 'pygraphviz' for graph visualization."
echo "To install 'pygraphviz', you will need to install Graphviz development libraries."
echo "On Ubuntu/Debian, run: sudo apt-get install graphviz graphviz-dev"
echo "On macOS, run: brew install graphviz"
echo "On Windows, follow instructions at: https://pygraphviz.github.io/documentation/stable/install.html"
echo "This setup script will attempt to install 'pygraphviz' now."
echo

pip install pygraphviz || echo "Warning: 'pygraphviz' installation failed. Please ensure Graphviz is installed and try again, or install with 'pip install pygraphviz'"

echo
echo
echo "Setup complete. To activate the virtual environment in the future, run:"
echo "source $VENV_PATH/bin/activate"
echo "If you encountered any issues, please refer to the README.md for troubleshooting tips."
