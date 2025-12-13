# Makefile for rotation module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lm

# Source files
TARGETS = rotation_detector
SOURCES = rotation.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

# Default target
all: $(TARGETS)

# Build executable
rotation_detector: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGETS) $(LDFLAGS)

# Compile object files with header dependencies
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGETS)
	rm -f ../../outputs/rotation/rotated_*
