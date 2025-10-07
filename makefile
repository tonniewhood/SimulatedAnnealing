
# --- Project Layout ---
PROJECT      := Lab04
PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR    := $(PROJECT_ROOT)/build
DATA_DIR     := $(PROJECT_ROOT)/data
INCLUDE_DIR  := $(PROJECT_ROOT)/include
SRC_DIR      := $(PROJECT_ROOT)/src
BIN_DIR      := $(BUILD_DIR)/bin

# --- Source Files ---
SRCS         := $(wildcard $(SRC_DIR)/*.cpp)

# --- Compiler and Flags ---
CXX          := g++
CXX_STD      := c++17
CXXFLAGS     := -Wall -Wextra -std=$(CXX_STD) -I$(INCLUDE_DIR) -DDATA_DIR=\"$(DATA_DIR)\" -DPROJECT_ROOT=\"$(PROJECT_ROOT)\"
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

OBJS   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.$(OBJEXT),$(SRCS))
TARGET := $(BIN_DIR)/$(PROJECT)

ifdef ARGS
	INPUT  := $(word 1, $(ARGS))
	OUTPUT := $(word 2, $(ARGS))
	FLAGS  := $(wordlist 3, $(words $(ARGS)), $(ARGS))
endif

# --- Declare command line arguments ---
INPUT          ?= input.txt
OUTPUT         ?= output.txt
MUTATION_MEHOD ?= naive
FLAGS          ?= "--mutation-method="$(MUTATION_MEHOD)
# FLAGS          ?= ""
ARGS           ?= $(INPUT) $(OUTPUT) $(FLAGS)

# --- Utilities ---
MKDIR_P := mkdir -p
RM_RF   := rm -rf


# --- Build Rules ---
.PHONY: all run clean help

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "\nBuild complete: $(TARGET)\n"

$(BUILD_DIR)/%.$(OBJEXT): $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

$(BIN_DIR):
	$(MKDIR_P) $(BIN_DIR)

clean:
	$(RM_RF) $(BUILD_DIR)

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
