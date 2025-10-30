# Makefile for neural network module

CC = gcc
CFLAGS = -O2 -Wall -lm

# Source files
SOURCE = neuralnetwork.c
TARGET = mlp_xor
MODEL = mlp_xor.model

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) -o $(TARGET) $(CFLAGS)

# Train the neural network
train: $(TARGET)
	./$(TARGET)

# Retrain the neural network
retrain: $(TARGET)
	rm -f $(MODEL)
	./$(TARGET)

# Test the neural network
test: $(TARGET)
	@if [ "$(word 2,$(MAKECMDGOALS))" = "" ] || [ "$(word 3,$(MAKECMDGOALS))" = "" ]; then \
		echo "Usage: make test <a> <b>"; \
		exit 1; \
	fi; \
	a=$(word 2,$(MAKECMDGOALS)); \
	b=$(word 3,$(MAKECMDGOALS)); \
	echo "Testing with input: $$a $$b"; \
	./$(TARGET) $$a $$b; \
	exit 0

%:
	@:

# Clean build files
clean:
	rm -f $(TARGET) $(MODEL)
