# Makefile for letter recognition project
CC = gcc
CFLAGS = -O2 -Wall
CFLAGS_TRAIN = -O2 -Wall -DBUILD_TRAINING_MAIN
LIBS_TRAIN = -lm `pkg-config --cflags --libs gdk-pixbuf-2.0`
LIBS_LABEL = -lm

# Directories
DATASET_DIR = dataset
CELLS1_DIR = $(DATASET_DIR)/cells1
CELLS2_DIR = $(DATASET_DIR)/cells2
LIST1_DIR = $(DATASET_DIR)/list1
LIST2_DIR = $(DATASET_DIR)/list2

# Label files
LABEL_CELLS1 = labels_cells1.txt
LABEL_CELLS2 = labels_cells2.txt
LABEL_LIST1 = labels_list1.txt
LABEL_LIST2 = labels_list2.txt
LABEL_ALL = labels_all.txt

# Model file
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
all: $(LABEL_TOOL) $(TRAIN_TOOL) $(RECOGNIZE_TOOL)

# Build labeling tool
$(LABEL_TOOL): label_cells.c stb_image.h
	$(CC) $(CFLAGS) label_cells.c -o $(LABEL_TOOL) $(LIBS_LABEL)

# Build training tool
$(TRAIN_TOOL): enhanced_training.c neuralnetwork.c neuralnetwork.h
	$(CC) $(CFLAGS_TRAIN) enhanced_training.c neuralnetwork.c -o $(TRAIN_TOOL) $(LIBS_TRAIN)

# Build recognition tool
$(RECOGNIZE_TOOL): file_creator.c neuralnetwork.c neuralnetwork.h
	$(CC) $(CFLAGS) file_creator.c neuralnetwork.c -o $(RECOGNIZE_TOOL) $(LIBS_TRAIN)

# Label cells1 directory
label-cells1: $(LABEL_TOOL)
	@if [ ! -d "$(CELLS1_DIR)" ]; then \
		echo "Error: Directory $(CELLS1_DIR) not found!"; \
		exit 1; \
	fi
	@echo "Labeling cells1..."
	./$(LABEL_TOOL) $(CELLS1_DIR) $(LABEL_CELLS1)

# Label cells2 directory
label-cells2: $(LABEL_TOOL)
	@if [ ! -d "$(CELLS2_DIR)" ]; then \
		echo "Error: Directory $(CELLS2_DIR) not found!"; \
		exit 1; \
	fi
	@echo "Labeling cells2..."
	./$(LABEL_TOOL) $(CELLS2_DIR) $(LABEL_CELLS2)

# Label list1 directory (word_X subdirectories with char_XXX.png)
label-list1: $(LABEL_TOOL)
	@if [ ! -d "$(LIST1_DIR)" ]; then \
		echo "Error: Directory $(LIST1_DIR) not found!"; \
		exit 1; \
	fi
	@echo "Labeling list1 (letters from word extraction)..."
	./$(LABEL_TOOL) $(LIST1_DIR) $(LABEL_LIST1)

# Label list2 directory
label-list2: $(LABEL_TOOL)
	@if [ ! -d "$(LIST2_DIR)" ]; then \
		echo "Error: Directory $(LIST2_DIR) not found!"; \
		exit 1; \
	fi
	@echo "Labeling list2 (letters from word extraction)..."
	./$(LABEL_TOOL) $(LIST2_DIR) $(LABEL_LIST2)

# Label all datasets
label-all: label-cells1 label-cells2 label-list1 label-list2
	@echo "All datasets labeled!"

# Merge all label files into one
merge-labels:
	@echo "Merging all label files into $(LABEL_ALL)..."
	@cat $(LABEL_CELLS1) $(LABEL_CELLS2) $(LABEL_LIST1) $(LABEL_LIST2) 2>/dev/null > $(LABEL_ALL) || true
	@echo "Merged labels: $$(wc -l < $(LABEL_ALL)) total labels"

# Train model with all labels
train: $(TRAIN_TOOL)
	@if [ ! -f "$(LABEL_ALL)" ]; then \
		echo "Error: $(LABEL_ALL) not found!"; \
		echo "Run 'make merge-labels' first to merge all label files."; \
		exit 1; \
	fi
	@echo "Training with labels from $(LABEL_ALL)..."
	./$(TRAIN_TOOL) $(LABEL_ALL) $(MODEL_FILE)

# Train with specific label file
train-with: $(TRAIN_TOOL)
	@if [ -z "$(LABELS)" ]; then \
		echo "Error: Specify label file with LABELS=filename"; \
		echo "Example: make train-with LABELS=labels_cells1.txt"; \
		exit 1; \
	fi
	@if [ ! -f "$(LABELS)" ]; then \
		echo "Error: $(LABELS) not found!"; \
		exit 1; \
	fi
	@echo "Training with labels from $(LABELS)..."
	./$(TRAIN_TOOL) $(LABELS) $(MODEL_FILE)

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
	rm -f $(LABEL_CELLS1) $(LABEL_CELLS2) $(LABEL_LIST1) $(LABEL_LIST2) $(LABEL_ALL) $(MODEL_FILE)
	@echo "Deleted all label files and model"

# Clean recognition outputs
clean-outputs:
	rm -f $(GRID_OUTPUT) $(WORDS_OUTPUT)
	@echo "Deleted recognition output files"

# Show label statistics for a specific file
stats:
	@if [ -z "$(LABELS)" ]; then \
		LABELS=$(LABEL_ALL); \
	fi; \
	if [ -f "$$LABELS" ]; then \
		echo "=== Label Statistics for $$LABELS ==="; \
		echo "Total labels: $$(wc -l < $$LABELS)"; \
		echo ""; \
		echo "Per letter:"; \
		cut -d' ' -f1 $$LABELS | sort | uniq -c | sort -rn; \
	else \
		echo "No labels file found: $$LABELS"; \
	fi

# Show stats for all label files
stats-all:
	@echo "=== Statistics for all label files ==="
	@for file in $(LABEL_CELLS1) $(LABEL_CELLS2) $(LABEL_LIST1) $(LABEL_LIST2) $(LABEL_ALL); do \
		if [ -f "$$file" ]; then \
			echo ""; \
			echo "File: $$file"; \
			echo "Labels: $$(wc -l < $$file)"; \
		fi; \
	done

# Clean executables and outputs
clean:
	rm -f $(LABEL_TOOL) $(TRAIN_TOOL) $(RECOGNIZE_TOOL) $(GRID_OUTPUT) $(WORDS_OUTPUT)

# Clean everything including labels
clean-all: clean
	rm -f $(LABEL_CELLS1) $(LABEL_CELLS2) $(LABEL_LIST1) $(LABEL_LIST2) $(LABEL_ALL) $(MODEL_FILE)

# Test dependencies
test-deps:
	@echo "Testing dependencies..."
	@pkg-config --exists gdk-pixbuf-2.0 && echo "✓ gdk-pixbuf-2.0" || echo "✗ gdk-pixbuf-2.0 NOT found"
	@which $(CC) > /dev/null && echo "✓ gcc" || echo "✗ gcc NOT found"

# Check dataset structure
check-dataset:
	@echo "Checking dataset structure..."
	@for dir in $(CELLS1_DIR) $(CELLS2_DIR) $(LIST1_DIR) $(LIST2_DIR); do \
		if [ -d "$$dir" ]; then \
			count=$$(find "$$dir" -name "*.png" | wc -l); \
			echo "✓ $$dir : $$count images"; \
		else \
			echo "✗ $$dir : NOT FOUND"; \
		fi; \
	done

# Show help
help:
	@echo "Letter Recognition Training System"
	@echo ""
	@echo "Quick Start:"
	@echo "  1. make check-dataset  - Verify dataset structure"
	@echo "  2. make label-all      - Label all datasets"
	@echo "  3. make merge-labels   - Merge all labels into one file"
	@echo "  4. make train          - Train the model"
	@echo "  5. make recognize      - Recognize letters"
	@echo ""
	@echo "Labeling Commands:"
	@echo "  make label-cells1   - Label cells from grid 1"
	@echo "  make label-cells2   - Label cells from grid 2"
	@echo "  make label-list1    - Label letters from word list 1"
	@echo "  make label-list2    - Label letters from word list 2"
	@echo "  make label-all      - Label all datasets"
	@echo ""
	@echo "Training Commands:"
	@echo "  make merge-labels        - Merge all label files"
	@echo "  make train               - Train with all labels"
	@echo "  make train-with LABELS=file.txt - Train with specific file"
	@echo "  make retrain             - Delete model and retrain"
	@echo ""
	@echo "Statistics & Info:"
	@echo "  make stats               - Show stats for merged labels"
	@echo "  make stats LABELS=file   - Show stats for specific file"
	@echo "  make stats-all           - Show stats for all files"
	@echo "  make check-dataset       - Check dataset structure"
	@echo ""
	@echo "Recognition:"
	@echo "  make recognize      - Run recognition"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean          - Remove built executables"
	@echo "  make clean-outputs  - Remove recognition outputs"
	@echo "  make reset          - Delete all labels and model"
	@echo "  make clean-all      - Remove everything"
	@echo ""
	@echo "Files:"
	@echo "  Datasets:  $(DATASET_DIR)/{cells1,cells2,list1,list2}/"
	@echo "  Labels:    $(LABEL_CELLS1), $(LABEL_CELLS2), $(LABEL_LIST1), $(LABEL_LIST2)"
	@echo "  Merged:    $(LABEL_ALL)"
	@echo "  Model:     $(MODEL_FILE)"
