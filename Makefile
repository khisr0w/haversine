# Unity build

# Directories
SRC_DIR := src
BIN_DIR := binary
EXECUTABLE := haversine 
DEBUG_DIR := $(BIN_DIR)/debug
RELEASE_DIR := $(BIN_DIR)/release

# Compiler and flags
CC := clang
CFLAGS_COMMON := -fno-caret-diagnostics -Wno-null-dereference -DPLT_LINUX -lm #/EHa /nologo /FC /Zo /WX /W4 /Gm- /wd5208 /wd4505
CFLAGS_DEBUG := -g #/Od /MTd /Z7 /Zo /DDEBUG
CFLAGS_RELEASE := #/O2 /Oi /MT /DRELEASE

ifeq ($(OS),Windows_NT)
CC := cl
EXECUTABLE := haversine.exe
CFLAGS_COMMON := /EHa /nologo /FC /Zo /WX /W4 /Gm- /wd5208 /wd4505 /wd4201 /DPLT_WIN /D_CRT_SECURE_NO_WARNINGS
CFLAGS_DEBUG := /Od /MTd /Z7 /Zo /DDEBUG
CFLAGS_RELEASE := /Od /Oi /MT /DRELEASE
endif

# Source files
# SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
SOURCES := $(SRC_DIR)/main.c

# Object files
# OBJECTS_DEBUG := $(patsubst $(SRC_DIR)/%.cpp,$(DEBUG_DIR)/%.obj,$(SOURCES))
# OBJECTS_RELEASE := $(patsubst $(SRC_DIR)/%.cpp,$(RELEASE_DIR)/%.obj,$(SOURCES))

OBJECTS_DEBUG := $(DEBUG_DIR)/main.obj
OBJECTS_RELEASE := $(RELEASE_DIR)/main.obj

# Executables
EXECUTABLE_DEUBG := $(DEBUG_DIR)/$(EXECUTABLE)
EXECUTABLE_RELEASE := $(RELEASE_DIR)/$(EXECUTABLE)

# Phony targets
.PHONY: all clean debug release

# Default target
all: debug release

# Debug target
debug: $(DEBUG_DIR) $(OBJECTS_DEBUG) $(EXECUTABLE_DEUBG)
	@echo Debug build complete...

$(DEBUG_DIR):
	@echo Starting debug build...
ifeq ($(OS),Windows_NT)
	@if not exist "$(DEBUG_DIR)" ( mkdir "$(DEBUG_DIR)" )
else
	@mkdir -p $(DEBUG_DIR)
endif

$(OBJECTS_DEBUG): $(SOURCES)
	@echo "    [Debug] Compiling Objects..."

ifeq ($(OS),Windows_NT)
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) /Fo$@ /c $<
else
	@$(CC) -fdiagnostics-absolute-paths $(CFLAGS_COMMON) $(CFLAGS_DEBUG) -c $< -o $@ 
endif

# Link debug executable
$(EXECUTABLE_DEUBG): $(OBJECTS_DEBUG)
	@echo "    [Debug] Linking Objects for executable..."
ifeq ($(OS),Windows_NT)
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) /Fe$@ $^
else
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $^ -o $@
endif


# Release target
release: $(RELEASE_DIR) $(OBJECTS_RELEASE) $(EXECUTABLE_RELEASE)
	@echo Release build complete...

$(RELEASE_DIR):
	@echo Starting release build...
ifeq ($(OS),Windows_NT)
	@if not exist "$(RELEASE_DIR)" ( mkdir "$(RELEASE_DIR)" )
else
	@mkdir -p $(RELEASE_DIR)
endif

$(OBJECTS_RELEASE): $(SOURCES)
	@echo "    Compiling Objects..."
ifeq ($(OS),Windows_NT)
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) /Fo$@ /c $<
else
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) -c $< -o $@ 
endif

# Link release executable
$(EXECUTABLE_RELEASE): $(OBJECTS_RELEASE)
	@echo "    Linking Objects for executable..."
ifeq ($(OS),Windows_NT)
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) /Fe$@ $^
else
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $^ -o $@
endif

# Clean target
clean:
	@rm -rf $(BIN_DIR)/*
	@echo Cleaned!
