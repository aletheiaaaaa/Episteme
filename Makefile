CXX         := g++
BASE_FLAGS  := -std=c++23

ifdef DEBUG
    OPT_FLAGS := -g -O0 -fsanitize=address,undefined
else
    OPT_FLAGS := -O3 -flto=auto
endif

CXXFLAGS := $(BASE_FLAGS) $(OPT_FLAGS)

SRC_DIR     := src
OBJ_DIR     := obj
BIN_DIR     := .
DEFAULT_NET := 1024_simple_v3.bin
EVALFILE    ?= $(DEFAULT_NET)
NETS_REPO   := https://github.com/aletheiaaaaa/episteme-nets
NET_FILENAME := $(notdir $(DEFAULT_NET))
NET_URL     := $(NETS_REPO)/releases/download/episteme_v1/$(NET_FILENAME)

ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
    ifdef MSYSTEM

        NULL_DEV     := /dev/null
        RM_RF         = rm -rf $(1)
        MKDIR_TARGET  = mkdir -p $(@D)
        SRCS         := $(shell find $(SRC_DIR) -name '*.cpp')
    else

        NULL_DEV     := nul
        RM_RF         = powershell -NoProfile -Command \
                            "if (Test-Path '$(subst /,\,$(1))') { Remove-Item -Recurse -Force -Path '$(subst /,\,$(1))' }"
        MKDIR_TARGET  = if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"

        SRCS         := $(addprefix $(SRC_DIR)/,$(subst \,/,$(shell \
                            powershell -NoProfile -Command \
                            "(Get-ChildItem -Recurse -Path '$(SRC_DIR)' -Filter '*.cpp' -Name)")))
    endif
else

    EXE_EXT      :=
    NULL_DEV     := /dev/null
    RM_RF         = rm -rf $(1)
    MKDIR_TARGET  = mkdir -p $(@D)
    SRCS         := $(shell find $(SRC_DIR) -name '*.cpp')
endif

OBJS   := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
EXE    ?= episteme
TARGET := $(BIN_DIR)/$(EXE)$(EXE_EXT)

ifdef ARCH
    DETECTED_ARCH := $(ARCH)
else
    _NATIVE_MACROS := $(shell $(CXX) -march=native -dM -E - < $(NULL_DEV) 2>$(NULL_DEV))
    ifneq ($(findstring __AVX512VNNI__,$(_NATIVE_MACROS)),)
        DETECTED_ARCH := avx512_vnni
    else ifneq ($(findstring __AVX2__,$(_NATIVE_MACROS)),)
        DETECTED_ARCH := avx2
    else ifneq ($(findstring __SSSE3__,$(_NATIVE_MACROS)),)
        DETECTED_ARCH := ssse3
    else
        DETECTED_ARCH := generic
    endif
endif

ifeq ($(DETECTED_ARCH),avx512_vnni)
    ARCH_FLAGS := -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vnni
    ARCH_DEF   := -DUSE_AVX512 -DUSE_VNNI
    $(info Building with AVX-512 + VNNI support)
else ifeq ($(DETECTED_ARCH),avx2)
    ARCH_FLAGS := -mavx2 -mfma
    ARCH_DEF   := -DUSE_AVX2
    $(info Building with AVX2 support)
else ifeq ($(DETECTED_ARCH),ssse3)
    ARCH_FLAGS := -mssse3
    ARCH_DEF   := -DUSE_SSSE3
    $(info Building with SSSE3 support)
else
    ARCH_FLAGS :=
    ARCH_DEF   :=
    $(info Building with generic SSE/SSE2 support)
endif

CXXFLAGS += $(ARCH_FLAGS) $(ARCH_DEF) -DEVALFILE=\"$(EVALFILE)\"

all: $(TARGET)

$(TARGET): $(DEFAULT_NET) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lstdc++exp

$(DEFAULT_NET):
	@echo "Network file $(DEFAULT_NET) not found. Downloading..."
ifeq ($(OS)_$(if $(MSYSTEM),msys,cmd),Windows_NT_cmd)
	powershell -NoProfile -ExecutionPolicy Bypass -Command \
		"Invoke-WebRequest -Uri '$(NET_URL)' -OutFile '$(DEFAULT_NET)' -UseBasicParsing; \
		 if ($$LASTEXITCODE) { Write-Error 'Download failed: $(NET_URL)'; exit 1 }"
else
	@if command -v curl >/dev/null 2>&1; then \
		curl -L -o $@ $(NET_URL) || { echo "Failed to download $(NET_URL)"; exit 1; }; \
	elif command -v wget >/dev/null 2>&1; then \
		wget -O $@ $(NET_URL) || { echo "Failed to download $(NET_URL)"; exit 1; }; \
	else \
		echo "Neither curl nor wget found. Please install one or download manually:"; \
		echo "  $(NET_URL)"; exit 1; \
	fi
endif
	@echo "Downloaded $(DEFAULT_NET) successfully"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR_TARGET)
	$(CXX) $(CXXFLAGS) -c $< -o $@

download-net:
	@echo "Downloading network file..."
	-$(call RM_RF,$(DEFAULT_NET))
	$(MAKE) $(DEFAULT_NET)

clean:
	-$(call RM_RF,$(OBJ_DIR))
	-$(call RM_RF,$(TARGET))

clean-all:
	-$(call RM_RF,$(OBJ_DIR))
	-$(call RM_RF,$(TARGET))
	-$(call RM_RF,$(DEFAULT_NET))

rebuild:     clean all
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
