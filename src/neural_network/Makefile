# Makefile for letter recognition project
CC = gcc
CFLAGS = -O2 -Wall
CFLAGS_TRAIN = -O2 -Wall -DBUILD_TRAINING_MAIN
LIBS_TRAIN = -lm `pkg-config --cflags --libs gdk-pixbuf-2.0`
LIBS_LABEL = -lm

# Directories
DATASET_DIR = dataset

# Define dataset types and their numbers
CELLS_NUMS = 1 2
LIST_NUMS = 1 2

# Generate directory paths
CELLS_DIRS = $(foreach n,$(CELLS_NUMS),$(DATASET_DIR)/cells$(n))
LIST_DIRS = $(foreach n,$(LIST_NUMS),$(DATASET_DIR)/list$(n))
ALL_DIRS = $(CELLS_DIRS) $(LIST_DIRS)

# Generate label file names
LABEL_CELLS = $(foreach n,$(CELLS_NUMS),labels_cells$(n).txt)
LABEL_LISTS = $(foreach n,$(LIST_NUMS),labels_list$(n).txt)
ALL_LABEL_FILES = $(LABEL_CELLS) $(LABEL_LISTS)
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

.PHONY: all clean train retrain help recognize

# Build all tools
all: $(TRAIN_TOOL) $(RECOGNIZE_TOOL)

# Build labeling tool
$(LABEL_TOOL): label_cells.c stb_image.h
	$(CC) $(CFLAGS) label_cells.c -o $(LABEL_TOOL) $(LIBS_LABEL)

# Build training tool
$(TRAIN_TOOL): enhanced_training.c neuralnetwork.c neuralnetwork.h
	$(CC) $(CFLAGS_TRAIN) enhanced_training.c neuralnetwork.c -o $(TRAIN_TOOL) $(LIBS_TRAIN)

# Build recognition tool
$(RECOGNIZE_TOOL): file_creator.c neuralnetwork.c neuralnetwork.h
	$(CC) $(CFLAGS) file_creator.c neuralnetwork.c -o $(RECOGNIZE_TOOL) $(LIBS_TRAIN)

# Generate label targets for cells directories dynamically
define LABEL_CELLS_TEMPLATE
label-cells$(1): $(LABEL_TOOL)
	@if [ ! -d "$(DATASET_DIR)/cells$(1)" ]; then \
		echo "Error: Directory $(DATASET_DIR)/cells$(1) not found!"; \
		exit 1; \
	fi
	@echo "Labeling cells$(1)..."
	./$(LABEL_TOOL) $(DATASET_DIR)/cells$(1) labels_cells$(1).txt
endef

# Generate label targets for list directories dynamically
define LABEL_LIST_TEMPLATE
label-list$(1): $(LABEL_TOOL)
	@if [ ! -d "$(DATASET_DIR)/list$(1)" ]; then \
		echo "Error: Directory $(DATASET_DIR)/list$(1) not found!"; \
		exit 1; \
	fi
	@echo "Labeling list$(1) (letters from word extraction)..."
	./$(LABEL_TOOL) $(DATASET_DIR)/list$(1) labels_list$(1).txt
endef

# Instantiate templates for all cells and list numbers
$(foreach n,$(CELLS_NUMS),$(eval $(call LABEL_CELLS_TEMPLATE,$(n))))
$(foreach n,$(LIST_NUMS),$(eval $(call LABEL_LIST_TEMPLATE,$(n))))

# Label all datasets
label-all: $(addprefix label-cells,$(CELLS_NUMS)) $(addprefix label-list,$(LIST_NUMS))
	@echo "All datasets labeled!"

# Merge all label files into one
merge-labels:
	@echo "Merging all label files into $(LABEL_ALL)..."
	@cat $(ALL_LABEL_FILES) 2>/dev/null > $(LABEL_ALL) || true
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
	rm -f $(ALL_LABEL_FILES) $(LABEL_ALL) $(MODEL_FILE)
	@echo "Deleted all label files and model"

# Clean recognition outputs
clean-outputs:
	rm -f $(GRID_OUTPUT) $(WORDS_OUTPUT)
	@echo "Deleted recognition output files"

# Clean executables and outputs
clean:
	rm -f $(LABEL_TOOL) $(TRAIN_TOOL) $(RECOGNIZE_TOOL) $(GRID_OUTPUT) $(WORDS_OUTPUT)

# Clean everything including labels
clean-all: clean
	rm -f $(ALL_LABEL_FILES) $(LABEL_ALL) $(MODEL_FILE)

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
	@echo "  1. make check-dataset  - Verify dataset structure"
	@echo "  2. make label-all      - Label all datasets"
	@echo "  3. make merge-labels   - Merge all labels into one file"
	@echo "  4. make train          - Train the model"
	@echo "  5. make recognize      - Recognize letters"
	@echo ""
	@echo "Labeling Commands:"
	@echo "  make label-cells1   - Label cells from grid 1"
	@echo "  make label-cells2   - Label cells from grid 2"
	@echo "  make label-cells3   - Label cells from grid 3"
	@echo "  make label-cells4   - Label cells from grid 4"
	@echo "  make label-list1    - Label letters from word list 1"
	@echo "  make label-list2    - Label letters from word list 2"
	@echo "  make label-list3    - Label letters from word list 3"
	@echo "  make label-list4    - Label letters from word list 4"
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
	@echo "  Datasets:  $(DATASET_DIR)/{cells1-4,list1-4}/"
	@echo "  Labels:    $(ALL_LABEL_FILES)"
	@echo "  Merged:    $(LABEL_ALL)"
	@echo "  Model:     $(MODEL_FILE)"
