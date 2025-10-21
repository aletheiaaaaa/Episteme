CXX       := g++
CXXFLAGS  := -std=c++23 -O3 -flto=auto

SRC_DIR   := src
OBJ_DIR   := ./obj
BIN_DIR   := .

DEFAULT_NET := aquamarine_1024_128.bin
EVALFILE    ?= $(DEFAULT_NET)

# Network repository configuration
NETS_REPO := https://github.com/aletheiaaaaa/episteme-nets
NET_FILENAME := $(notdir $(DEFAULT_NET))
NET_BASENAME := $(basename $(NET_FILENAME))
NET_URL := $(NETS_REPO)/releases/download/$(NET_BASENAME)/$(NET_FILENAME)

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

CXXFLAGS  += $(ARCH_FLAGS) $(ARCH_DEF) -DEVALFILE=\"$(EVALFILE)\"

EXE     ?= episteme
TARGET  := $(BIN_DIR)/$(EXE)

SRCS    := $(shell find $(SRC_DIR) -name '*.cpp' ! -name 'preprocess.cpp')
OBJS    := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Debug target
debug:
	$(MAKE) DEBUG=1

# Main target - depends on network file and object files
$(TARGET): $(DEFAULT_NET) $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Download network file if it doesn't exist
$(DEFAULT_NET):
	@echo "Network file $(DEFAULT_NET) not found. Downloading..."
	@if command -v wget >/dev/null 2>&1; then \
		wget -O $(DEFAULT_NET) $(NET_URL) || { echo "Failed to download $(NET_URL)"; exit 1; }; \
	elif command -v curl >/dev/null 2>&1; then \
		curl -L -o $(DEFAULT_NET) $(NET_URL) || { echo "Failed to download $(NET_URL)"; exit 1; }; \
	else \
		echo "Neither wget nor curl found. Please install one of them or download $(NET_URL) manually to $(DEFAULT_NET)"; \
		exit 1; \
	fi
	@echo "Downloaded $(DEFAULT_NET) successfully"

# Compile sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Force download network (useful for updates)
download-net:
	@echo "Downloading network file..."
	@rm -f $(DEFAULT_NET)
	@$(MAKE) $(DEFAULT_NET)

# Clean build artifacts but keep network file
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Clean everything including network file
clean-all:
	rm -rf $(OBJ_DIR) $(TARGET) $(DEFAULT_NET)

# Rebuild without re-downloading network
rebuild: clean all

# Rebuild including fresh network download
rebuild-all: clean-all all

.PHONY: all debug clean clean-all rebuild rebuild-all download-net