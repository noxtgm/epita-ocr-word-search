# Makefile for detection module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 `pkg-config --cflags gtk+-3.0` -lm 
LDFLAGS = `pkg-config --libs gtk+-3.0` -lm	

# Grid detection source files
GRID_TARGETS = detect
GRID_SOURCES = detect.c main.c
GRID_OBJECTS = $(GRID_SOURCES:.c=.o)


.PHONY: all clean

# Default target
all: $(GRID_TARGETS) $(LIST_TARGETS)

# Build the grid detection executable
$(GRID_TARGETS): $(GRID_SOURCES)
	$(CC) $(GRID_SOURCES) -o $(GRID_TARGETS) $(LDFLAGS) $(CFLAGS)

# Build the list detection executable
$(LIST_TARGETS): $(LIST_SOURCES)
	$(CC) $(LIST_SOURCES) -o $(LIST_TARGETS) $(LDFLAGS) $(CFLAGS)

# Compile object files with header dependencies
%.o: %.c detect.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(GRID_TARGETS) $(GRID_OBJECTS) $(LIST_TARGETS) $(LIST_OBJECTS)
	rm -rf ../../outputs/list_detection/word_stats.txt
	rm -rf ../../outputs/list_detection/word_*/
	rm -rf ../../outputs/grid_detection/grid_index.txt
	rm -rf ../../outputs/grid_detection/row_*/
	rm -rf ../../outputs/grid_detection/index.txt
