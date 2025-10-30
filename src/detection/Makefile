CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 `pkg-config --cflags gtk+-3.0` -lm 
LIBS = `pkg-config --libs gtk+-3.0` -lm	

# Main detection tool
MAIN_TARGET = detect_grid
MAIN_SOURCES = detection.c main.c
MAIN_OBJECTS = $(MAIN_SOURCES:.c=.o)

# Word detection list tool
LIST_TARGET = detection_list
LIST_SOURCE = detection_list.c

# Your output folder
OUTPUT_DIR = output
OUTPUT_DIL = output_dir

# Default target
all: $(MAIN_TARGET) $(LIST_TARGET)

# Custom "detection" target builds both programs and ensures output folder exists
detection: $(MAIN_TARGET) $(LIST_TARGET) $(OUTPUT_DIR)
	@echo "[INFO] Both programs compiled. Output folder ready."

# Ensure your output folder exists
$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)
	@echo "[INFO] Created folder: $(OUTPUT_DIR)"

# Build the main detection tool
$(MAIN_TARGET): $(MAIN_SOURCES)
	$(CC) $(MAIN_SOURCES) -o $(MAIN_TARGET) $(LIBS) $(CFLAGS)

# Build the word detection list tool
$(LIST_TARGET): $(LIST_SOURCE) $(OUTPUT_DIR)
	$(CC) $(LIST_SOURCE) -o $(LIST_TARGET) $(LIBS) $(CFLAGS)

# Compile source files
%.o: %.c detection.h
	$(CC) $(CFLAGS) -c $< -o $@

# Run the main detection tool
detect: $(MAIN_TARGET)
	@echo "Usage: ./detect_grid <image_file>"
	@echo "Example: ./detect_grid wordsearch.jpg"

# Clean build artifacts
clean:
	rm -f $(MAIN_OBJECTS) $(MAIN_TARGET) $(LIST_TARGET)
	rm -rf $(OUTPUT_DIR)
	rm -rf $(OUTPUT_DIL)
	rm -rf detect_grid.exe
	rm -rf output_dir
	rm -rf *.png

del:
	rm -rf *.png
	rm -rf $(OUTPUT_DIL) 
	rm -rf $(OUTPUT_DIR)
	rm -rf output_dir

# Install dependencies (if needed)
install-deps:
	@echo "Installing required packages..."
	@echo "For Ubuntu/Debian: sudo apt-get install libgtk-3-dev"
	@echo "For CentOS/RHEL: sudo dnf install gtk3-devel"
	@echo "For macOS: brew install gtk+3"
	@echo "For Arch Linux: sudo pacman -S gtk3"
	@echo "For Windows (MSYS2): pacman -S mingw-w64-x86_64-gtk3"

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all test clean install-deps debug detection

# Check if GTK3 is available
check-gtk:
	@pkg-config --exists gtk+-3.0 && echo "✓ GTK3 is available (version: `pkg-config --modversion gtk+-3.0`)" || echo "✗ GTK3 not found"

# Check dependencies
check-deps: check-gtk
	@echo "Checking all dependencies..."
	@pkg-config --exists gdk-pixbuf-2.0 && echo "✓ GdkPixbuf: `pkg-config --modversion gdk-pixbuf-2.0`" || echo "✗ GdkPixbuf not found"
	@pkg-config --exists glib-2.0 && echo "✓ GLib: `pkg-config --modversion glib-2.0`" || echo "✗ GLib not found"
