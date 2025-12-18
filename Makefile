# ============================================================================
# FreeRTOS PC Scheduler Makefile
# Supports both Windows (MinGW) and Linux (GCC) builds
# ============================================================================

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
	DETECTED_OS := Windows
else
	DETECTED_OS := $(UNAME_S)
endif

# Compiler and Flags
CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -I./src -I./FreeRTOS/include -I./FreeRTOS/portable/MSVC-MingW

# Platform-specific flags
ifeq ($(DETECTED_OS),Linux)
	CFLAGS += -pthread
	LDFLAGS := -lpthread -lrt
	PORT_DIR := FreeRTOS/portable/ThirdParty/GCC/Posix
	CFLAGS += -I./$(PORT_DIR)
	PORT_SOURCE := $(PORT_DIR)/port.c
else
	# Windows (MinGW)
	LDFLAGS := -pthread -lwinmm -lws2_32
	PORT_DIR := FreeRTOS/portable/MSVC-MingW
	PORT_SOURCE := $(PORT_DIR)/port.c
endif

# Output executable
ifeq ($(DETECTED_OS),Windows)
	EXECUTABLE := freertos_sim.exe
else
	EXECUTABLE := freertos_sim
endif

# Source files
SOURCES := \
	src/main.c \
	src/scheduler.c \
	FreeRTOS/source/tasks.c \
	FreeRTOS/source/queue.c \
	FreeRTOS/source/list.c \
	FreeRTOS/source/timers.c \
	FreeRTOS/source/event_groups.c \
	FreeRTOS/portable/MemMang/heap_4.c \
	$(PORT_SOURCE)

# Object files
OBJECTS := $(SOURCES:.c=.o)

# Build directory
BUILD_DIR := build
BUILD_OBJECTS := $(patsubst %,$(BUILD_DIR)/%,$(OBJECTS))

# Phony targets
.PHONY: all clean help

# Default target
all: $(EXECUTABLE)

# Build executable
$(EXECUTABLE): $(BUILD_OBJECTS) | $(BUILD_DIR)
	@echo "$@ baglaniyor..."
	@$(CC) $(BUILD_OBJECTS) $(LDFLAGS) -o $@
	@echo "Derleme tamamlandi: $@"

# Compile source files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "$< derleniyor..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Derleme dosyalari temizleniyor..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(EXECUTABLE)
	@echo "Temizlik tamamlandi."

# Help
help:
	@echo "FreeRTOS PC Scheduler Makefile"
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Example:"
	@echo "  make              # Compile the project"
	@echo "  ./$(EXECUTABLE) giris.txt  # Run with input file"

# Debug: show detected OS
debug:
	@echo "Detected OS: $(DETECTED_OS)"
	@echo "Compiler: $(CC)"
	@echo "Executable: $(EXECUTABLE)"
	@echo "Port source: $(PORT_SOURCE)"
