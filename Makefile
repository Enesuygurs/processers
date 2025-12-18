# FreeRTOS PC Scheduler - Makefile
# 4 Seviyeli Oncelikli Gorev Siralayici Simulasyonu

CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -O2 -g
LDFLAGS = -pthread -lrt

# Dizinler
SRC_DIR = src
BUILD_DIR = build
FREERTOS_DIR = FreeRTOS
FREERTOS_SRC = $(FREERTOS_DIR)/source
FREERTOS_INC = $(FREERTOS_DIR)/include
FREERTOS_PORT = $(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix
FREERTOS_MEMMANG = $(FREERTOS_DIR)/portable/MemMang

# Include dizinleri
INCLUDES = -I$(SRC_DIR) \
           -I$(FREERTOS_INC) \
           -I$(FREERTOS_PORT) \
           -I$(FREERTOS_PORT)/utils

# FreeRTOS kaynak dosyalari
FREERTOS_SOURCES = $(FREERTOS_SRC)/tasks.c \
                   $(FREERTOS_SRC)/queue.c \
                   $(FREERTOS_SRC)/list.c \
                   $(FREERTOS_SRC)/timers.c \
                   $(FREERTOS_SRC)/event_groups.c \
                   $(FREERTOS_PORT)/port.c \
                   $(FREERTOS_PORT)/utils/wait_for_event.c \
                   $(FREERTOS_MEMMANG)/heap_4.c

# Uygulama kaynak dosyalari
APP_SOURCES = $(SRC_DIR)/main.c \
              $(SRC_DIR)/scheduler.c \
              $(SRC_DIR)/tasks.c

# Tum kaynaklar
SOURCES = $(APP_SOURCES) $(FREERTOS_SOURCES)

# Object dosyalari
APP_OBJECTS = $(BUILD_DIR)/main.o \
              $(BUILD_DIR)/scheduler.o \
              $(BUILD_DIR)/app_tasks.o

FREERTOS_OBJECTS = $(BUILD_DIR)/freertos_tasks.o \
                   $(BUILD_DIR)/queue.o \
                   $(BUILD_DIR)/list.o \
                   $(BUILD_DIR)/timers.o \
                   $(BUILD_DIR)/event_groups.o \
                   $(BUILD_DIR)/port.o \
                   $(BUILD_DIR)/wait_for_event.o \
                   $(BUILD_DIR)/heap_4.o

OBJECTS = $(APP_OBJECTS) $(FREERTOS_OBJECTS)

TARGET = freertos_sim

.PHONY: all clean run help dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Derleme tamamlandi: $(TARGET)"

# Uygulama object dosyalari
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/scheduler.h $(SRC_DIR)/FreeRTOSConfig.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/scheduler.o: $(SRC_DIR)/scheduler.c $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/app_tasks.o: $(SRC_DIR)/tasks.c $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# FreeRTOS object dosyalari
$(BUILD_DIR)/freertos_tasks.o: $(FREERTOS_SRC)/tasks.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/queue.o: $(FREERTOS_SRC)/queue.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/list.o: $(FREERTOS_SRC)/list.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/timers.o: $(FREERTOS_SRC)/timers.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/event_groups.o: $(FREERTOS_SRC)/event_groups.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/port.o: $(FREERTOS_PORT)/port.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/wait_for_event.o: $(FREERTOS_PORT)/utils/wait_for_event.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/heap_4.o: $(FREERTOS_MEMMANG)/heap_4.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: all
	./$(TARGET) giris.txt

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET).exe

help:
	@echo "=================================================="
	@echo "FreeRTOS PC Scheduler - Makefile Kullanim Rehberi"
	@echo "=================================================="
	@echo ""
	@echo "Kullanim: make [hedef]"
	@echo ""
	@echo "Hedefler:"
	@echo "  all   - Projeyi derle (varsayilan)"
	@echo "  run   - Derle ve giris.txt ile calistir"
	@echo "  clean - Derleme dosyalarini temizle"
	@echo "  help  - Bu yardim mesajini goster"
	@echo ""
	@echo "Not: Bu proje Linux veya WSL ortaminda calistirilmalidir."
