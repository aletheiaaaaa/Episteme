CXX       := g++
CXXFLAGS  := -std=c++23 -O3 -flto=auto

SRC_DIR   := src
OBJ_DIR   := ./obj
BIN_DIR   := .

# Debug mode
ifdef DEBUG
	CXXFLAGS := -std=c++23 -O0 -g -fsanitize=address,undefined -DDEBUG
else
	CXXFLAGS := -std=c++23 -O3 -flto=auto
endif

# Allow architecture override via ARCH variable
ifdef ARCH
	DETECTED_ARCH := $(ARCH)
else
	DETECTED_ARCH := $(shell \
	if grep -q avx512_vnni /proc/cpuinfo 2>/dev/null || sysctl -a 2>/dev/null | grep -q avx512_vnni; then \
		echo "avx512_vnni"; \
	else \
		echo "unsupported"; \
	fi)
endif

# Architecture-specific flags
ifeq ($(DETECTED_ARCH),avx512_vnni)
	ARCH_FLAGS := -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vbmi2 -mavx512vnni
	ARCH_DEF := -DUSE_AVX512 -DUSE_VNNI
else
	$(error Unsupported CPU architecture. Minimum requirement: AVX-512 with VNNI support.)
endif

DEFAULT_NET := aquamarine.bin
EVALFILE    ?= $(DEFAULT_NET)
PROCESSED_NET := $(basename $(EVALFILE))_$(DETECTED_ARCH).bin

# Network repository configuration
NETS_REPO := https://github.com/aletheiaaaaa/episteme-nets
NET_FILENAME := $(notdir $(EVALFILE))
NET_URL := $(NETS_REPO)/releases/download/$(basename $(NET_FILENAME))/$(NET_FILENAME)

CXXFLAGS  += $(ARCH_FLAGS) $(ARCH_DEF) -DEVALFILE=\"$(PROCESSED_NET)\"

EXE     ?= episteme
TARGET  := $(BIN_DIR)/$(EXE)

SRCS    := $(shell find $(SRC_DIR) -name '*.cpp' ! -name 'preprocess.cpp')
OBJS    := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

PREPROCESS_SRC := $(SRC_DIR)/utils/preprocess.cpp
PREPROCESS_OBJ := $(OBJ_DIR)/utils/preprocess.o
PREPROCESS_EXE := $(BIN_DIR)/preprocess

# Default target
all: $(TARGET)

# Debug target
debug:
	$(MAKE) DEBUG=1

# Main target - depends on processed network file and object files
$(TARGET): $(PROCESSED_NET) $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Build preprocess tool
$(PREPROCESS_EXE): $(PREPROCESS_SRC)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Download network file if it doesn't exist
$(EVALFILE):
	@echo "Network file $(EVALFILE) not found. Downloading..."
	@if command -v wget >/dev/null 2>&1; then \
		wget -O $(EVALFILE) $(NET_URL) || { echo "Failed to download $(NET_URL)"; exit 1; }; \
	elif command -v curl >/dev/null 2>&1; then \
		curl -L -o $(EVALFILE) $(NET_URL) || { echo "Failed to download $(NET_URL)"; exit 1; }; \
	else \
		echo "Neither wget nor curl found. Please install one of them or download $(NET_URL) manually to $(EVALFILE)"; \
		exit 1; \
	fi
	@echo "Downloaded $(EVALFILE) successfully"

# Process the network file
$(PROCESSED_NET): $(EVALFILE) $(PREPROCESS_EXE)
	@echo "Processing $(EVALFILE) -> $(PROCESSED_NET)"
	$(PREPROCESS_EXE) $(EVALFILE) $(PROCESSED_NET)
	@rm -f $(EVALFILE)

# Compile sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Force download and process network (useful for updates)
download-net:
	@echo "Downloading and processing network file..."
	@rm -f $(EVALFILE) $(PROCESSED_NET)
	@$(MAKE) $(PROCESSED_NET)

# Clean build artifacts but keep processed network file
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(PREPROCESS_EXE)

# Clean everything including processed network file
clean-all:
	rm -rf $(OBJ_DIR) $(TARGET) $(EVALFILE) $(PROCESSED_NET) $(PREPROCESS_EXE)

# Rebuild without re-downloading network
rebuild: clean all

# Rebuild including fresh network download
rebuild-all: clean-all all

.PHONY: all debug clean clean-all rebuild rebuild-all download-net