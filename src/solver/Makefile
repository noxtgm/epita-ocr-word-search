# Makefile for solver module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99

# Source files
TARGETS = solver
SOURCES = solver.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

# Default target
all: $(TARGETS)

# Build executable
$(TARGETS): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGETS)

# Compile object files with header dependencies
%.o: %.c solver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGETS)
	rm -f grid
