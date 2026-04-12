CXX         := g++
BASE_FLAGS  := -std=c++23

ifdef DEBUG
    OPT_FLAGS := -g -O0 -fsanitize=address,undefined
else
    OPT_FLAGS := -O3 -flto=auto
endif

CXXFLAGS := $(BASE_FLAGS) $(OPT_FLAGS)

SRC_DIR   := src
OBJ_DIR   := ./obj
BIN_DIR   := .

DEFAULT_NET := 512_v0_05.bin
EVALFILE    ?= $(DEFAULT_NET)

NETS_REPO := https://github.com/aletheiaaaaa/episteme-nets
NET_FILENAME := $(notdir $(DEFAULT_NET))
NET_URL := $(NETS_REPO)/releases/latest/download/$(NET_FILENAME)

ifdef ARCH
    DETECTED_ARCH := $(ARCH)
else
    DETECTED_ARCH := $(shell \
	if grep -q avx512_vnni /proc/cpuinfo 2>/dev/null || sysctl -a 2>/dev/null | grep -q avx512_vnni; then \
		echo "avx512_vnni"; \
	elif grep -q avx2 /proc/cpuinfo 2>/dev/null || sysctl -a 2>/dev/null | grep -q avx2; then \
		echo "avx2"; \
	elif grep -q ssse3 /proc/cpuinfo 2>/dev/null || sysctl -a 2>/dev/null | grep -q ssse3; then \
		echo "ssse3"; \
	else \
		echo "generic"; \
	fi)
endif

ifeq ($(DETECTED_ARCH),avx512_vnni)
    ARCH_FLAGS := -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vnni
    ARCH_DEF := -DUSE_AVX512 -DUSE_VNNI
    $(info Building with AVX-512 support)
else ifeq ($(DETECTED_ARCH),avx2)
    ARCH_FLAGS := -mavx2 -mfma
    ARCH_DEF := -DUSE_AVX2
    $(info Building with AVX2 support)
else ifeq ($(DETECTED_ARCH),ssse3)
    ARCH_FLAGS := -mssse3
    ARCH_DEF := -DUSE_SSSE3
    $(info Building with SSSE3 support)
else
    ARCH_FLAGS :=
    ARCH_DEF :=
    $(info Building with generic SSE/SSE2 support)
endif

CXXFLAGS  += $(ARCH_FLAGS) $(ARCH_DEF) -DEVALFILE=\"$(EVALFILE)\"

EXE     ?= episteme
TARGET  := $(BIN_DIR)/$(EXE)

SRCS    := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS    := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(DEFAULT_NET) $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

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

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

download-net:
	@echo "Downloading network file..."
	@rm -f $(DEFAULT_NET)
	@$(MAKE) $(DEFAULT_NET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

clean-all:
	rm -rf $(OBJ_DIR) $(TARGET) $(DEFAULT_NET)

rebuild: clean all

rebuild-all: clean-all all

avx2:
	$(MAKE) ARCH=avx2

avx512_vnni:
	$(MAKE) ARCH=avx512_vnni

ssse3:
	$(MAKE) ARCH=ssse3

generic:
	$(MAKE) ARCH=generic

debug:
	$(MAKE) DEBUG=1

show-arch:
	@echo "Detected architecture: $(DETECTED_ARCH)"
	@echo "Compiler flags: $(ARCH_FLAGS) $(ARCH_DEF)"

.PHONY: all clean clean-all rebuild rebuild-all download-net avx2 avx512_vnni ssse3 generic show-arch debug