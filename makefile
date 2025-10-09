
# --- Project Layout ---
PROJECT      := Lab04
PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR    := $(PROJECT_ROOT)/build
DATA_DIR     := $(PROJECT_ROOT)/data
INCLUDE_DIR  := $(PROJECT_ROOT)/include
SRC_DIR      := $(PROJECT_ROOT)/src
BIN_DIR      := $(BUILD_DIR)/bin

# --- Source Files ---
SRCS_BASE     := $(filter-out $(SRC_DIR)/pyViz.cpp, $(wildcard $(SRC_DIR)/*.cpp))

# --- Compiler Detection and Configuration ---
# Check for MSVC first (Windows environment)
ifeq ($(OS),Windows_NT)
    # Check if we're in a Visual Studio environment
    ifdef VCINSTALLDIR
        COMPILER := msvc
    else
        # Try to detect MinGW or other Windows compilers
        ifneq ($(shell where cl 2>nul),)
            COMPILER := msvc
        else
            COMPILER := gcc
        endif
    endif
    EXE_EXT := .exe
    PATH_SEP := \\
else
    COMPILER := gcc
    EXE_EXT :=
    PATH_SEP := /
endif

# Allow manual compiler override
ifdef USE_MSVC
    COMPILER := msvc
endif

# --- Compiler-Specific Settings ---
CXX_STD      := c++17

ifeq ($(COMPILER),msvc)
    # MSVC Compiler Settings
    CXX          := cl
    CPPFLAGS     := /I"$(INCLUDE_DIR)" /DDATA_DIR=\"$(DATA_DIR)\" /DPROJECT_ROOT=\"$(PROJECT_ROOT)\"
    CXXFLAGS     := /std:$(CXX_STD) /EHsc /W3
    LDFLAGS      := 
    LDLIBS       := 
    OBJEXT       := obj
    DEBUGFLAGS   := /Od /Zi /DEBUG
    RELFLAGS     := /O2 /DNDEBUG
    OUT_FLAG     := /Fe:
    OBJ_FLAG     := /Fo:
    LINK_OUT     := /OUT:
else
    # GCC/Clang Compiler Settings (default)
    CXX          := g++
    CPPFLAGS     := -I$(INCLUDE_DIR) -DDATA_DIR=\"$(DATA_DIR)\" -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"
    CXXFLAGS     := -Wall -Wextra -std=$(CXX_STD)
    LDFLAGS      :=
    LDLIBS       :=
    OBJEXT       := o
    DEBUGFLAGS   := -g -O0
    RELFLAGS     := -O3
    OUT_FLAG     := -o 
    OBJ_FLAG     := -o 
    LINK_OUT     := -o 
endif


# --- Determine build type ---
BUILD ?= debug
ifeq ($(BUILD), debug)
	CXXFLAGS += $(DEBUGFLAGS)
else
	CXXFLAGS += $(RELFLAGS)
endif

USE_PYTHON ?= 1
ifeq ($(USE_PYTHON),1)
  # --- Determine Python availability ---
  ifeq ($(COMPILER),msvc)
    # Windows/MSVC Python detection
    PYTHON3 := $(shell where python 2>nul)
    PYTHON  := $(PYTHON3)
    # For Windows, we'll use a different approach to get Python flags
    PYCONFIG := 
  else
    # Unix-like Python detection
    PYTHON3 := $(shell command -v python3 2> /dev/null)
    PYTHON  := $(shell command -v python 2> /dev/null)
    PYCONFIG := $(shell command -v python3-config 2> /dev/null || command -v python-config 2> /dev/null)
  endif
  
  ifeq ($(COMPILER),msvc)
    # MSVC Python detection and configuration
    ifeq ($(PYTHON3),)
      HAVE_PYTHON := 0
      $(warning No Python installation found; building without Python integration)
    else
      HAVE_PYTHON := 1
      # Get Python installation info for MSVC
      PYTHON_VERSION := $(shell python -c "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')" 2>nul)
      PYTHON_PATH := $(shell python -c "import sys; print(sys.exec_prefix)" 2>nul)
      ifeq ($(PYTHON_VERSION),)
        HAVE_PYTHON := 0
        $(warning Could not determine Python version; building without Python integration)
      else
        PYTHON_INCLUDES := /I"$(PYTHON_PATH)/include"
        PYTHON_LDFLAGS := /LIBPATH:"$(PYTHON_PATH)/libs"
        PYTHON_LDLIBS := python$(PYTHON_VERSION).lib
      endif
    endif
  else
    # Unix-like Python configuration
    ifeq ($(PYCONFIG),)
      $(warning python-config not found; building without Python integration)
      PYTHON3 :=
      PYTHON  :=
      HAVE_PYTHON := 0
    else
      ifeq ($(PYTHON3)$(PYTHON),)  # none found
        HAVE_PYTHON := 0
        $(warning No Python installation found; building without Python integration)
      else
        HAVE_PYTHON := 1
        PYTHON_INCLUDES := $(shell $(PYCONFIG) --includes)
        PYTHON_LDFLAGS  := $(shell $(PYCONFIG) --embed --ldflags 2>/dev/null)
        ifeq ($(PYTHON_LDFLAGS),)
          PYTHON_LDFLAGS  := $(shell $(PYCONFIG) --ldflags 2>/dev/null)
        endif
        PYTHON_LDLIBS := $(filter -l%, $(PYTHON_LDFLAGS))
      endif
    endif
  endif

  # --- Virtual environment check (cross-platform) ---
  ifeq ($(COMPILER),msvc)
    VENV_CHECK := $(shell if exist "$(PROJECT_ROOT)\venv" ( echo found ) else if exist "$(PROJECT_ROOT)\.venv" ( echo found ) else ( echo notfound ))
  else
    VENV_CHECK := $(shell [ -d "$(PROJECT_ROOT)/venv" ] || [ -d "$(PROJECT_ROOT)/.venv" ] && echo found || echo notfound)
  endif
  
  ifeq ($(VENV_CHECK),notfound)
    $(info No virtual environment found at $(PROJECT_ROOT)/venv or $(PROJECT_ROOT)/.venv)
    $(info Consider running 'setup.sh' found at the project root)
    $(info Or create manually with 'python -m venv venv' or 'python -m venv .venv')
  endif
  
  ifeq ($(HAVE_PYTHON),1)
    ifeq ($(COMPILER),msvc)
      CPPFLAGS += $(PYTHON_INCLUDES) /DHAVE_PYTHON=1
      LDFLAGS  += $(PYTHON_LDFLAGS)
      LDLIBS   += $(PYTHON_LDLIBS)
    else
      CPPFLAGS += $(PYTHON_INCLUDES) -DHAVE_PYTHON=1
      LDFLAGS  += $(filter -L% -Wl%, $(PYTHON_LDFLAGS))
      LDLIBS   += $(PYTHON_LDLIBS)
    endif
  endif
  
  SRCS       := $(SRCS_BASE) $(SRC_DIR)/pyViz.cpp  
  # --- If we're using Python, ask to setup a virtual environment ---
  ifeq ($(shell [ -d "$(PROJECT_ROOT)/venv" ] && echo -n || echo -y),y)
  	$(warning No virtual environment found; consider running 'python3 -m venv venv' in the project root)
  endif
else
  SRCS	      := $(SRCS_BASE)
  HAVE_PYTHON := 0
endif

# --- Build Targets ---
OBJS   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.$(OBJEXT),$(SRCS))
TARGET := $(BIN_DIR)/$(PROJECT)$(EXE_EXT)

# --- Utilities ---
ifeq ($(COMPILER),msvc)
    MKDIR_P := if not exist
    RM_RF   := rmdir /s /q
    MKDIR_CMD = $(MKDIR_P) "$(1)" mkdir "$(1)"
    RM_CMD = if exist "$(1)" $(RM_RF) "$(1)"
else
    MKDIR_P := mkdir -p
    RM_RF   := rm -rf
    MKDIR_CMD = $(MKDIR_P) $(1)
    RM_CMD = $(RM_RF) $(1)
endif


# --- Build Rules ---
.PHONY: all run run-full run-analysis clean help setup

all: $(TARGET)

ifeq ($(COMPILER),msvc)
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) $(LINK_OUT)$@ $(LDFLAGS) $(LDLIBS)
	@echo.
	@echo Build complete: $(TARGET)
	@echo.

$(BUILD_DIR)/%.$(OBJEXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) /c $< $(OBJ_FLAG)$@
else
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) $(OUT_FLAG)$@ $(LDFLAGS) $(LDLIBS)
	@echo "\nBuild complete: $(TARGET)\n"

$(BUILD_DIR)/%.$(OBJEXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< $(OBJ_FLAG)$@
endif

$(BUILD_DIR):
	$(call MKDIR_CMD,$(BUILD_DIR))

$(BIN_DIR):
	$(call MKDIR_CMD,$(BIN_DIR))

clean:
	$(call RM_CMD,$(BUILD_DIR))

ifdef ARGS
	INPUT  := $(word 1, $(ARGS))
	OUTPUT := $(word 2, $(ARGS))
	FLAGS  := $(wordlist 3, $(words $(ARGS)), $(ARGS))
endif

# --- Declare command line arguments ---
INPUT           ?= input.txt
OUTPUT          ?= output.txt
MUTATION_METHOD ?= shift
PLOT_TYPE       ?= 
SAVE_FIGS       ?= false
FIGURE_PATH     ?= $(PROJECT_ROOT)/
RUN_ANALYSIS    ?= false
MUTATION_FLAG   ?= --mutation-method=$(MUTATION_METHOD)
PLOT_FLAG       ?= --plot-type=$(PLOT_TYPE)
SAVE_FLAG       ?= --save-figures=$(SAVE_FIGS)
FIGURE_FLAG     ?= --figure-path=$(FIGURE_PATH)
ANALYSIS_FLAG   ?= --run-analysis=$(RUN_ANALYSIS)
FLAGS ?= \
	$(if $(MUTATION_METHOD),$(MUTATION_FLAG),) \
	$(if $(PLOT_TYPE),$(PLOT_FLAG),) \
	$(if $(SAVE_FIGS),$(SAVE_FLAG),) \
	$(if $(FIGURE_PATH),$(FIGURE_FLAG),) \
	$(if $(RUN_ANALYSIS),$(ANALYSIS_FLAG),)
ARGS            ?= $(INPUT) $(OUTPUT) $(FLAGS)

run: all
ifeq ($(COMPILER),msvc)
	@echo.
	@echo Running $(TARGET) with arguments: $(ARGS)
	@echo.
	@$(TARGET) $(ARGS)
else
	@echo "\nRunning $(TARGET) with arguments: $(ARGS)\n\n"
	@$(TARGET) $(ARGS)
endif

run-full: all
ifeq ($(COMPILER),msvc)
	@echo.
	@echo Running $(TARGET) with arguments: data/input.txt output.txt --mutation-method=shift --plot-type=graph --save-figures=true --figure-path=figures/
	@echo.
	@$(TARGET) data/input.txt output.txt --mutation-method=shift --plot-type=all --save-figures=true --figure-path=figures/
else
	@echo "\nRunning $(TARGET) with arguments: data/input.txt output.txt --mutation-method=shift --plot-type=graph --save-figures=true --figure-path=figures/\n\n"
	@$(TARGET) data/input.txt output.txt --mutation-method=shift --plot-type=all --save-figures=true --figure-path=figures/
endif

run-analysis: all
ifeq ($(COMPILER),msvc)
	@echo.
	@echo Running annealing analysis with $(TARGET)...
	@echo.
	@$(TARGET) data/input.txt output.txt --run-analysis=true
else
	@echo "\nRunning annealing analysis with $(TARGET)...\n\n"
	@$(TARGET) data/input.txt output.txt --run-analysis=true
endif

setup:
	@echo "Running project setup..."
	@chmod +x $(PROJECT_ROOT)/setup.sh
	@$(PROJECT_ROOT)/setup.sh

help:
ifeq ($(COMPILER),msvc)
	@echo Makefile for $(PROJECT) (Using MSVC)
	@echo Usage:
	@echo   make [BUILD=debug^|release]   Build the project (default is debug)
	@echo   make [USE_MSVC=1]            Force MSVC compiler usage
	@echo   make setup                   Run initial project setup (Python venv, dependencies)
	@echo   make run                     Build and run the project
	@echo   make clean                   Clean build artifacts
	@echo   make help                    Show this help message
	@echo.
	@echo Current compiler: $(CXX)
else
	@echo "Makefile for $(PROJECT) (Using GCC/Clang)"
	@echo "Usage:"
	@echo "  make [BUILD=debug|release]   Build the project (default is debug)"
	@echo "  make [USE_MSVC=1]            Force MSVC compiler usage (Windows)"
	@echo "  make setup                   Run initial project setup (Python venv, dependencies)"
	@echo "  make run                     Build and run the project"
	@echo "  make clean                   Clean build artifacts"
	@echo "  make help                    Show this help message"
	@echo ""
	@echo "Current compiler: $(CXX)"
endif
