# Makefile for solver module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99

# Source files
SOURCE = solver.c
OBJECTS = $(SOURCE:.c=.o)
TARGET = solver

.PHONY: all clean

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Compile object files with header dependencies
%.o: %.c solver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
