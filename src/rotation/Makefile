# Makefile for rotation module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 `pkg-config --cflags gdk-pixbuf-2.0`
LDFLAGS = `pkg-config --libs gdk-pixbuf-2.0` -lm

# Source files
SOURCE = rotation.c
OBJECTS = $(SOURCE:.c=.o)
TARGET = rotate

.PHONY: all clean

# Default target
all: $(TARGET)

# Build executable
rotation: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile object files with header dependencies
%.o: %.c rotation.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f ../../outputs/rotated/rotated_*
