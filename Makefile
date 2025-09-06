CXX       := g++
CXXFLAGS  := -std=c++23 -O3 -flto=auto -mavx2

SRC_DIR   := src
OBJ_DIR   := ./obj
BIN_DIR   := .

DEFAULT_NET := ./512_v0_05.bin
EVALFILE    ?= $(DEFAULT_NET)

# Network repository configuration
NETS_REPO := https://github.com/aletheiaaaaa/episteme-nets
NET_FILENAME := $(notdir $(DEFAULT_NET))
NET_URL := $(NETS_REPO)/releases/latest/download/$(NET_FILENAME)

CXXFLAGS  += -DEVALFILE=\"$(EVALFILE)\"

EXE     ?= episteme
TARGET  := $(BIN_DIR)/$(EXE)

SRCS    := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS    := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Main target - depends on network file and object files
$(TARGET): $(OBJS)
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

.PHONY: all clean clean-all rebuild rebuild-all download-net