# Makefile for letter recognition project
CC = gcc
CFLAGS = -O2 -Wall
CFLAGS_TRAIN = -O2 -Wall -DBUILD_TRAINING_MAIN
LIBS_TRAIN = -lm `pkg-config --cflags --libs gdk-pixbuf-2.0`
LIBS_LABEL = -lm

# Directories
CELL_DIR = ../../outputs/grid_detection/cells/
LABEL_FILE = labels.txt
MODEL_FILE = letter_model.bin

# Output files
GRID_OUTPUT = ../../outputs/recognized_files/recognized_grid.txt
WORDS_OUTPUT = ../../outputs/recognized_files/recognized_words.txt

# Targets
LABEL_TOOL = label_cells
TRAIN_TOOL = train_mlp
RECOGNIZE_TOOL = file_creator

.PHONY: all clean label train retrain help recognize

# Build all tools
all: $(LABEL_TOOL) $(TRAIN_TOOL) $(VERIFY_TOOL) $(RECOGNIZE_TOOL)

# Build labeling tool
$(LABEL_TOOL): label_cells.c stb_image.h
	$(CC) $(CFLAGS) label_cells.c -o $(LABEL_TOOL) $(LIBS_LABEL)

# Build training tool
$(TRAIN_TOOL): enhanced_training.c neuralnetwork.c neuralnetwork.h
	$(CC) $(CFLAGS_TRAIN) enhanced_training.c neuralnetwork.c -o $(TRAIN_TOOL) $(LIBS_TRAIN)


# Build recognition tool
$(RECOGNIZE_TOOL): file_creator.c neuralnetwork.c neuralnetwork.h
	$(CC) $(CFLAGS) file_creator.c neuralnetwork.c -o $(RECOGNIZE_TOOL) $(LIBS_TRAIN)

# Label images (interactive)
label: $(LABEL_TOOL)
	@if [ ! -d "$(CELL_DIR)" ]; then \
		echo "Error: Cell directory $(CELL_DIR) not found!"; \
		exit 1; \
	fi
	@echo "Starting labeling session..."
	@echo "Labels will be appended to $(LABEL_FILE)"
	./$(LABEL_TOOL) $(CELL_DIR) $(LABEL_FILE)

# Label word list cells
label-words: $(LABEL_TOOL)
	@echo "Labeling word directories..."
	@for dir in ../../outputs/list_detection/word_*/; do \
		if [ -d "$$dir" ]; then \
			echo "Processing $$dir"; \
			./$(LABEL_TOOL) "$$dir" $(LABEL_FILE); \
		fi; \
	done
	@echo "Word labeling complete!"

# Label both grid and words
label-all: $(LABEL_TOOL)
	@echo "Labeling grid cells..."
	./$(LABEL_TOOL) $(CELL_DIR) $(LABEL_FILE)
	@echo "Labeling word cells..."
	@$(MAKE) label-words

# Train model
train: $(TRAIN_TOOL)
	@if [ ! -f "$(LABEL_FILE)" ]; then \
		echo "Error: $(LABEL_FILE) not found!"; \
		echo "Run 'make label' first to create labels."; \
		exit 1; \
	fi
	@echo "Training with labels from $(LABEL_FILE)..."
	./$(TRAIN_TOOL) $(LABEL_FILE) $(MODEL_FILE)

# Recognize letters in grid and word lists
recognize: $(RECOGNIZE_TOOL)
	@if [ ! -f "$(MODEL_FILE)" ]; then \
		echo "Error: $(MODEL_FILE) not found!"; \
		echo "Run 'make train' first to create the model."; \
		exit 1; \
	fi
	@echo "Running recognition on grid and word lists..."
	./$(RECOGNIZE_TOOL) $(MODEL_FILE) $(GRID_OUTPUT) $(WORDS_OUTPUT)

# Retrain from scratch
retrain:
	rm -f $(MODEL_FILE)
	$(MAKE) train

# Start fresh (delete labels and model)
reset:
	rm -f $(LABEL_FILE) $(MODEL_FILE)
	@echo "Deleted $(LABEL_FILE) and $(MODEL_FILE)"

# Clean recognition outputs
clean-outputs:
	rm -f $(GRID_OUTPUT) $(WORDS_OUTPUT)
	@echo "Deleted recognition output files"

# Show label statistics
stats:
	@if [ -f "$(LABEL_FILE)" ]; then \
		echo "=== Label Statistics ==="; \
		echo "Total labels: $$(wc -l < $(LABEL_FILE))"; \
		echo ""; \
		echo "Per letter:"; \
		cut -d' ' -f1 $(LABEL_FILE) | sort | uniq -c | sort -rn; \
	else \
		echo "No labels file found."; \
	fi



# Clean executables and model
clean:
	rm -f $(LABEL_TOOL) $(TRAIN_TOOL) $(RECOGNIZE_TOOL) $(GRID_OUTPUT) $(WORDS_OUTPUT)

# Clean everything including labels and outputs
clean-all: clean
	rm -f $(LABEL_FILE) $(MODEL_FILE)

# Test dependencies
test-deps:
	@echo "Testing dependencies..."
	@pkg-config --exists gdk-pixbuf-2.0 && echo "✓ gdk-pixbuf-2.0" || echo "✗ gdk-pixbuf-2.0 NOT found"
	@which $(CC) > /dev/null && echo "✓ gcc" || echo "✗ gcc NOT found"

# Show help
help:
	@echo "Letter Recognition Training System"
	@echo ""
	@echo "Quick Start:"
	@echo "  1. make              - Build all tools"
	@echo "  2. make label        - Label your cell images"
	@echo "  3. make train        - Train the model"
	@echo "  4. make recognize    - Recognize letters in grid and words"
	@echo ""
	@echo "All Targets:"
	@echo "  make              - Build all tools"
	@echo "  make label        - Start interactive labeling session"
	@echo "  make train        - Train model from labels"
	@echo "  make recognize    - Run recognition on grid and word lists"
	@echo "  make retrain      - Delete model and retrain"
	@echo "  make stats        - Show label statistics"
	@echo "  make reset        - Delete labels and model (start over)"
	@echo "  make clean        - Remove built executables and model"
	@echo "  make clean-outputs - Remove recognition output files"
	@echo "  make clean-all    - Remove everything including labels"
	@echo "  make test-deps    - Check dependencies"
	@echo ""
	@echo "Files:"
	@echo "  Input cells:  $(CELL_DIR)*.png"
	@echo "  Label file:   $(LABEL_FILE)"
	@echo "  Model file:   $(MODEL_FILE)"
	@echo "  Grid output:  $(GRID_OUTPUT)"
	@echo "  Words output: $(WORDS_OUTPUT)"
