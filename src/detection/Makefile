CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -lm $(shell pkg-config --cflags sdl2)
LIBS = $(shell pkg-config --libs sdl2) -lm
TARGET = wordsearch_detector
ADVANCED_TARGET = advanced_detector
MAIN_TARGET = detect_grid
SOURCES = detection.c test.c
ADVANCED_SOURCES = detection.c advanced_test.c
MAIN_SOURCES = detection.c main.c
OBJECTS = $(SOURCES:.c=.o)
ADVANCED_OBJECTS = $(ADVANCED_SOURCES:.c=.o)
MAIN_OBJECTS = $(MAIN_SOURCES:.c=.o)

# Default target
all: $(TARGET) $(ADVANCED_TARGET) $(MAIN_TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

# Build the advanced executable
$(ADVANCED_TARGET): $(ADVANCED_OBJECTS)
	$(CC) $(ADVANCED_OBJECTS) -o $(ADVANCED_TARGET) $(LIBS)

# Build the main detection tool
$(MAIN_TARGET): $(MAIN_OBJECTS)
	$(CC) $(MAIN_OBJECTS) -o $(MAIN_TARGET) $(LIBS)

# Compile source files
%.o: %.c detection.h
	$(CC) $(CFLAGS) -c $< -o $@

# Run the test
test: $(TARGET)
	./$(TARGET)

# Run the advanced test
test-advanced: $(ADVANCED_TARGET)
	./$(ADVANCED_TARGET)

# Run the main detection tool
detect: $(MAIN_TARGET)
	@echo "Usage: ./detect_grid <image_file>"
	@echo "Example: ./detect_grid wordsearch.jpg"

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(ADVANCED_OBJECTS) $(MAIN_OBJECTS) $(TARGET) $(ADVANCED_TARGET) $(MAIN_TARGET)

# Install dependencies (if needed)
install-deps:
	@echo "Installing required packages..."
	@echo "For Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-image-dev"
	@echo "For CentOS/RHEL: sudo yum install SDL2-devel SDL2_image-devel"
	@echo "For macOS: brew install sdl2 sdl2_image"
	@echo "For Arch Linux: sudo pacman -S sdl2 sdl2_image"

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG
release: $(TARGET)

.PHONY: all test clean install-deps debug release

# PNG support targets
png: CFLAGS += -DSDL2_IMAGE_AVAILABLE $(shell pkg-config --cflags SDL2_image 2>/dev/null || echo "")
png: LIBS += $(shell pkg-config --libs SDL2_image 2>/dev/null || echo "")
png: $(TARGET) $(ADVANCED_TARGET) $(MAIN_TARGET)

# Check if SDL2_image is available
check-png:
	@pkg-config --exists SDL2_image && echo "✓ SDL2_image is available - PNG support enabled" || echo "✗ SDL2_image not found - PNG support disabled"

# Install PNG support
install-png:
	@echo "Installing SDL2_image for PNG support..."
	@echo "Run: sudo apt-get install libsdl2-image-dev"
