CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -pthread

SRC_DIR = src
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/scheduler.c $(SRC_DIR)/tasks.c
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/tasks.o
TARGET = freertos_sim

.PHONY: all clean run help

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $@

$(BUILD_DIR)/scheduler.o: $(SRC_DIR)/scheduler.c $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/scheduler.c -o $@

$(BUILD_DIR)/tasks.o: $(SRC_DIR)/tasks.c $(SRC_DIR)/scheduler.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tasks.c -o $@

run: all
	./$(TARGET) giris.txt

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET).exe

help:
	@echo "Kullanim: make [hedef]"
	@echo "  all   - Projeyi derle"
	@echo "  run   - Derle ve calistir"
	@echo "  clean - Temizle"
