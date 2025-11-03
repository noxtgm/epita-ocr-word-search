# Makefile for neural network module

CC = gcc
CFLAGS = -O2 -Wall -lm

# Source files
TARGETS = mlp_xnor
SOURCES = neuralnetwork.c
MODELS = mlp_xnor.model

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

%:
	@:

# Clean build files
clean:
	rm -f $(TARGETS) $(MODELS)
