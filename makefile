
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

# --- Compiler and Flags ---
CXX          := g++
CXX_STD      := c++17
CPPFLAGS     := -I$(INCLUDE_DIR) -DDATA_DIR=\"$(DATA_DIR)\" -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"
CXXFLAGS     := -Wall -Wextra -std=$(CXX_STD)
LDFLAGS      :=
LDLIBS       :=
INCLUDES     := -I$(INCLUDE_DIR)
OBJEXT       := o
DEBUGFLAGS   := -g -O0
RELFLAGS     := -O3


# --- Determine build type ---
BUILD ?= debug
ifeq ($(BUILD), debug)
	CXXFLAGS += $(DEBUGFLAGS)
else
	CXXFLAGS += $(RELFLAGS)
endif

# --- Determine Python availability ---
PYTHON3 := $(shell command -v python3 2> /dev/null)
PYTHON  := $(shell command -v python 2> /dev/null)
PYCONFIG := $(shell command -v python3-config 2> /dev/null || command -v python-config 2> /dev/null)

ifeq ($(PYCONFIG),)
  $(warning python-config not found; building without Python integration)
  PYTHON3 :=
  PYTHON  :=
else
  ifeq ($(PYTHON3)$(PYTHON),)  # none found
    HAVE_PYTHON := 0
    $(warning No Python installation found; building without Python integration)
  else
    HAVE_PYTHON := 1
  endif
endif

ifeq ($(HAVE_PYTHON),1)
  PYTHON_INCLUDES := $(shell $(PYCONFIG) --includes)
  PYTHON_LDFLAGS  := $(shell $(PYCONFIG) --embed --ldflags 2>/dev/null)
  ifeq ($(PYTHON_LDFLAGS),)
    PYTHON_LDFLAGS  := $(shell $(PYCONFIG) --ldflags 2>/dev/null)
  endif

  CPPFLAGS   += $(PYTHON_INCLUDES) -DHAVE_PYTHON=1
  LDFLAGS	 += $(filter -L% -Wl%, $(PYTHON_LDFLAGS))
  LDLIBS	 += $(filter -l%, $(PYTHON_LDFLAGS))

  SRCS       := $(SRCS_BASE) $(SRC_DIR)/pyViz.cpp
else
  SRCS	     := $(SRCS_BASE)
endif

# --- Build Targets ---
OBJS   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.$(OBJEXT),$(SRCS))
TARGET := $(BIN_DIR)/$(PROJECT)

# --- Utilities ---
MKDIR_P := mkdir -p
RM_RF   := rm -rf


# --- Build Rules ---
.PHONY: all run clean help

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "\nBuild complete: $(TARGET)\n"

$(BUILD_DIR)/%.$(OBJEXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

clean:
	$(RM_RF) $(BUILD_DIR)

ifdef ARGS
	INPUT  := $(word 1, $(ARGS))
	OUTPUT := $(word 2, $(ARGS))
	FLAGS  := $(wordlist 3, $(words $(ARGS)), $(ARGS))
endif

# --- Declare command line arguments ---
INPUT           ?= input.txt
OUTPUT          ?= output.txt
MUTATION_METHOD ?= naive
PLOT_TYPE       ?= --plot-type=graph
# STATISTICS      ?= --statistics=true
STATISTICS      ?= 
FLAGS           ?= "--mutation-method="$(MUTATION_METHOD) $(PLOT_TYPE) $(STATISTICS)
ARGS            ?= $(INPUT) $(OUTPUT) $(FLAGS)

run: all
	@echo "\nRunning $(TARGET) with arguments: $(ARGS)\n\n"
	@$(TARGET) $(ARGS)

help:
	@echo "Makefile for $(PROJECT)"
	@echo "Usage:"
	@echo "  make [BUILD=debug|release]   Build the project (default is debug)"
	@echo "  make run                     Build and run the project"
	@echo "  make clean                   Clean build artifacts"
	@echo "  make help                    Show this help message"
