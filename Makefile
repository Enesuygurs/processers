# FreeRTOS PC Siralayici Makefile
# Windows (MinGW) ve Linux (GCC) derlemelerini destekler

# İşletim sistemini algıla
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
	DETECTED_OS := Windows
else
	DETECTED_OS := $(UNAME_S)
endif

# Derleyici ve bayraklar
CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -I./src -I./FreeRTOS/include

# Platform'a özel bayraklar
ifeq ($(DETECTED_OS),Linux)
	CFLAGS += -pthread
	LDFLAGS := -lpthread -lrt
	PORT_DIR := FreeRTOS/portable/ThirdParty/GCC/Posix
	CFLAGS += -I./$(PORT_DIR) -I./$(PORT_DIR)/utils
	PORT_SOURCE := $(PORT_DIR)/port.c
else
	# Windows (MinGW)
	CFLAGS += -I./FreeRTOS/portable/MSVC-MingW
	LDFLAGS := -pthread -lwinmm -lws2_32
	PORT_DIR := FreeRTOS/portable/MSVC-MingW
	PORT_SOURCE := $(PORT_DIR)/port.c
endif

# Çıktı yürütülebilir dosyası
ifeq ($(DETECTED_OS),Windows)
	EXECUTABLE := freertos_sim.exe
else
	EXECUTABLE := freertos_sim
endif

# Kaynak dosyaları
SOURCES := \
	src/main.c \
	src/scheduler.c \
	src/tasks.c \
	FreeRTOS/source/tasks.c \
	FreeRTOS/source/queue.c \
	FreeRTOS/source/list.c \
	FreeRTOS/source/timers.c \
	FreeRTOS/source/event_groups.c \
	FreeRTOS/portable/MemMang/heap_4.c \
	$(PORT_SOURCE)

# Linux derlemeleri için POSIX olay yardımcılarını ekle
ifeq ($(DETECTED_OS),Linux)
	SOURCES += FreeRTOS/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
endif

# Nesne dosyaları
OBJECTS := $(SOURCES:.c=.o)

# Derleme dizini
BUILD_DIR := build
BUILD_OBJECTS := $(patsubst %,$(BUILD_DIR)/%,$(OBJECTS))

# Sahte hedefler
.PHONY: all clean help

# Varsayılan hedef
all: $(EXECUTABLE)

# Yürütülebilir dosyayı oluştur
$(EXECUTABLE): $(BUILD_OBJECTS) | $(BUILD_DIR)
	@echo "$@ baglaniyor..."
	@$(CC) $(BUILD_OBJECTS) $(LDFLAGS) -o $@
	@echo "Derleme tamamlandi: $@"

# Kaynak dosyaları derle
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "$< derleniyor..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Derleme dizini oluştur
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Derleme dosyalarını temizle
clean:
	@echo "Derleme dosyalari temizleniyor..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(EXECUTABLE)
	@echo "Temizlik tamamlandi."

# Yardım
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
