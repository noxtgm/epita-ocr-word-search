# Makefile for solver module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99

# Source files
SOURCE = solver.c
OBJECTS = $(SOURCE:.c=.o)
TARGET = solver

.PHONY: all clean testgrid

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Compile object files with header dependencies
%.o: %.c solver.h
	$(CC) $(CFLAGS) -c $< -o $@

# Create test grid file
testgrid:
	@echo "HORIZONTAL" > grid
	@echo "DXRAHCLBGA" >> grid
	@echo "DIKCILEOKC" >> grid
	@echo "IGAJHYLYHI" >> grid
	@echo "HGFGODTIOT" >> grid
	@echo "GDLROWKBFR" >> grid
	@echo "PLNRDNERGE" >> grid
	@echo "JHAIDUAJGV" >> grid
	@echo "UKGFFOLLEH" >> grid

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f grid
