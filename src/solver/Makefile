# Makefile for solver module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99

# Source files
TARGETS = solver visualize
SOURCES = solver.c
VISUALIZE_SOURCES = visualize.c
OBJECTS = $(SOURCES:.c=.o)
VISUALIZE_OBJECTS = $(VISUALIZE_SOURCES:.c=.o)
VISUALIZE_CFLAGS = -Wall -Wextra -O2 -std=c99 `pkg-config --cflags gdk-pixbuf-2.0`
VISUALIZE_LDFLAGS = `pkg-config --libs gdk-pixbuf-2.0` -lm

.PHONY: all clean visualize

# Default target
all: $(TARGETS)

# Build solver executable
solver: $(OBJECTS)
	$(CC) $(OBJECTS) -o solver

# Build visualize executable
visualize: $(VISUALIZE_OBJECTS)
	$(CC) $(VISUALIZE_OBJECTS) -o visualize $(VISUALIZE_LDFLAGS)

# Compile solver object files with header dependencies
solver.o: solver.c solver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile visualize object files
visualize.o: visualize.c
	$(CC) $(VISUALIZE_CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(VISUALIZE_OBJECTS) $(TARGETS)
	rm -f grid
