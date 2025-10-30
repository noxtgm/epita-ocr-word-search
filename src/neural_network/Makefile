# Makefile for neural network module

CC = gcc
CFLAGS = -O2 -Wall -lm

# Source files
TARGETS = mlp_xor
SOURCES = neuralnetwork.c
MODELS = mlp_xor.model

.PHONY: all clean train retrain test

# Default target
all: $(TARGETS)

# Build executable
$(TARGETS): $(SOURCES)
	$(CC) $(SOURCES) -o $(TARGETS) $(CFLAGS)

# Train the neural network
train: $(TARGETS)
	./$(TARGETS)

# Retrain the neural network
retrain: $(TARGETS)
	rm -f $(MODELS)
	./$(TARGETS)

# Test the neural network
test: $(TARGETS)
	@if [ "$(word 2,$(MAKECMDGOALS))" = "" ] || [ "$(word 3,$(MAKECMDGOALS))" = "" ]; then \
		echo "Usage: make test <a> <b>"; \
		exit 1; \
	fi; \
	a=$(word 2,$(MAKECMDGOALS)); \
	b=$(word 3,$(MAKECMDGOALS)); \
	echo "Testing with input: $$a $$b"; \
	./$(TARGETS) $$a $$b; \
	exit 0

%:
	@:

# Clean build files
clean:
	rm -f $(TARGETS) $(MODELS)
