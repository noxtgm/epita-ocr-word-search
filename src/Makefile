# Makefile for OCR Word Search

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`

# Source files
TARGET = ocr_word_search
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

# Subdirectories
SUBDIRS = detection neural_network rotation solver

.PHONY: all clean clean-all

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean ui and other modules
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -rf ../outputs/*
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
