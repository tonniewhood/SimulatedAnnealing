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

IS_DRIVE_PATH := $(and \
  $(findstring :/,$(SHELL)), \
  $(if $(filter /%,$(SHELL)),,1) \
)

# --- Compiler Detection and Configuration ---
# Check for MSVC first (Windows environment)
ifeq ($(IS_DRIVE_PATH),1)
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
    SHELL := cmd.exe
    .SHELLFLAGS := /C
    winpath = $(subst /,\,$(1))
    MKDIR_CMD = if not exist "$(call winpath,$(1))" mkdir "$(call winpath,$(1))"
    RM_CMD    = if exist "$(call winpath,$(1))" rmdir /s /q "$(call winpath,$(1))"
else
    COMPILER := gcc
    EXE_EXT :=
    PATH_SEP := /
    SHELL := /usr/bin/bash
    .SHELLFLAGS := -c
    MKDIR_P := mkdir -p
    RM_RF   := rm -rf
    MKDIR_CMD = $(MKDIR_P) $(1)
    RM_CMD = $(RM_RF) $(1)
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
    COMPILE_FLAG := /c
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
    COMPILE_FLAG := -c
endif

SHELLTYPE := posix
ifeq ($(OS),Windows_NT)
  ifneq (,$(findstring powershell,$(SHELL)))
    SHELLTYPE := winsh
  else ifneq (,$(findstring pwsh,$(SHELL)))
    SHELLTYPE := winsh
  else ifneq (,$(findstring cmd,$(SHELL)))
    SHELLTYPE := winsh
  endif
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
  ifeq ($(SHELLTYPE),winsh)
    # Windows/MSVC Python detection
    PYTHON3 := $(word 1, $(shell where python 2>nul))
    PYTHON  := $(PYTHON3)
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
      else ifeq ($(COMPILER),msvc)
        PYTHON_INCLUDES := /I"$(PYTHON_PATH)/include"
        PYTHON_LDFLAGS  := /LIBPATH:"$(PYTHON_PATH)/libs"
        PYTHON_LDLIBS   := python$(PYTHON_VERSION).lib
      else

        PY_INC_DIR := $(shell $(PYTHON) -c "import sysconfig;print(sysconfig.get_config_var('INCLUDEPY') or '')")
        PY_LIBDIR  := $(shell $(PYTHON) -c "import sysconfig,sys;print(sysconfig.get_config_var('LIBDIR') or sysconfig.get_config_var('LIBPL') or sys.base_prefix + r'\libs')")
        PY_LIBNAME := $(shell $(PYTHON) -c "import sysconfig,os,sys;n=sysconfig.get_config_var('LDLIBRARY') or sysconfig.get_config_var('LIBRARY') or ('python'+str(sys.version_info.major)+str(sys.version_info.minor));print(os.path.splitext(os.path.basename(n))[0])")

        # Export GCC-style flags (match your POSIX names)
        PYTHON_INCLUDES := -I$(PY_INC_DIR)
        PYTHON_LDFLAGS  := -L$(PY_LIBDIR)
        PYTHON_LDLIBS   := -l$(PY_LIBNAME)
      endif
    endif
  else
    # Unix-like Python configuration
    PYTHON3 := $(shell command -v python3 2> /dev/null)
    PYTHON  := $(shell command -v python 2> /dev/null)
    PYCONFIG := $(shell command -v python3-config 2> /dev/null || command -v python-config 2> /dev/null)
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
      endif
    endif
  endif

  VENV_DIR :=$(firsword \
    $(wildcard $(PROJECT_ROOT)/venv) \
    $(wildcard $(PROJECT_ROOT)/.venv) )

  VENV_CHECK := $(if $(VENV_DIR),)

  ifeq ($(VENV_CHECK),notfound)
    $(info No virtual environment found at $(PROJECT_ROOT)/venv or $(PROJECT_ROOT)/.venv)
    $(info Consider running 'setup.sh' found at the project root)
    $(info Or create manually with 'python -m venv venv' or 'python -m venv .venv')
  endif

#   ifeq ($(SHELLTYPE),winsh)
    CXXFLAGS += -DVENV_DIR=\"$(PROJECT_ROOT)/venv\" -DSCRIPTS_DIR=\"$(PROJECT_ROOT)/scripts\"
#   endif

  ifeq ($(HAVE_PYTHON),1)
    ifeq ($(COMPILER),msvc)
      CPPFLAGS += $(PYTHON_INCLUDES) /DHAVE_PYTHON=1
      LDFLAGS  += $(PYTHON_LDFLAGS)
      LDLIBS   += $(PYTHON_LDLIBS)
    else
      CPPFLAGS += $(PYTHON_INCLUDES) -DHAVE_PYTHON=1
      LDFLAGS  += $(filter -L% -Wl%, $(PYTHON_LDFLAGS))
      LDLIBS   += $(filter -l% -lpython%, $(PYTHON_LDFLAGS))
      $(info $(PYTHON_LDFLAGS))
      $(info $(PYTHON_LDLIBS))
      
    endif
  endif

  SRCS       := $(SRCS_BASE) $(SRC_DIR)/pyViz.cpp
else
  SRCS        := $(SRCS_BASE)
  HAVE_PYTHON := 0
endif

# --- Build Targets ---
OBJS   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.$(OBJEXT),$(SRCS))
TARGET := $(BIN_DIR)/$(PROJECT)$(EXE_EXT)


# --- Build Rules ---
.PHONY: all release run run-full run-analysis clean help setup
.RECIPEPREFIX := >

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
> $(CXX) $(OBJS) $(LINK_OUT)$@ $(LDFLAGS) $(LDLIBS)
> $(echo)
> $(info Build complete: $(TARGET))
> $(info )

$(BUILD_DIR)/%.$(OBJEXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
> $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COMPILE_FLAG) $< $(OBJ_FLAG)$@

$(BUILD_DIR):
> $(call MKDIR_CMD,$(BUILD_DIR))

$(BIN_DIR):
> $(call MKDIR_CMD,$(BIN_DIR))

clean:
> $(call RM_CMD,$(BUILD_DIR))

ifdef ARGS
> INPUT  := $(word 1, $(ARGS))
> OUTPUT := $(word 2, $(ARGS))
> FLAGS  := $(wordlist 3, $(words $(ARGS)), $(ARGS))
endif

# --- Declare command line arguments ---
INPUT           ?= data/demo.txt
OUTPUT          ?= test_output.txt
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
> $(info )
> $(info Running $(TARGET) with arguments: $(ARGS))
> $(info )
> @$(TARGET) $(ARGS)

run-full: all
> $(info )
> $(info Running $(TARGET) with arguments: data/demo.txt output.txt --mutation-method=shift --plot-type=graph --save-figures=true --figure-path=figures/)
> $(info )
> @$(TARGET) data/demo.txt output.txt --mutation-method=shift --plot-type=all --save-figures=true --figure-path=figures/

run-analysis: all
> $(info )
> $(info Running annealing analysis with $(TARGET)...)
> $(info )
> @$(TARGET) data/demo.txt output.txt --run-analysis=true

setup:
> @echo "Running project setup..."
> @chmod +x $(PROJECT_ROOT)/setup.sh
> @$(PROJECT_ROOT)/setup.sh

help:
> $(info Makefile for $(PROJECT))
> $(info Usage:)
> $(info   make [BUILD=debug^|release]   Build the project (default is debug))
> $(info   make [USE_MSVC=1]            Force MSVC compiler usage)
> $(info   make setup                   Run initial project setup (Python venv, dependencies))
> $(info   make run                     Build and run the project)
> $(info   make clean                   Clean build artifacts)
> $(info   make help                    Show this help message)
> $(info )
> $(info Current compiler: $(CXX))
