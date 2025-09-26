# Word Search Grid Detection System (SDL2)

This project implements a computer vision system to detect and extract word search grids from images using SDL2 for robust image handling. The system can identify grid boundaries, determine grid dimensions, and locate individual cells within the grid.

## Features

- **SDL2 Integration**: Uses SDL2 for robust image processing and surface management
- **Grid Detection**: Automatically detects rectangular grid boundaries in images
- **Image Preprocessing**: Includes grayscale conversion, Gaussian blur, edge detection, and thresholding
- **Grid Analysis**: Determines grid dimensions (rows and columns)
- **Cell Extraction**: Identifies individual grid cells and their center coordinates
- **Letter Detection**: Framework for OCR integration (placeholder implementation)
- **Cross-platform**: Works on Linux, Windows, and macOS

## Project Structure

```
detection/
├── detection.h          # Header file with structures and function declarations
├── detection.c          # Main implementation of detection algorithms
├── main.c              # Main program - takes image as command line argument
├── test.c              # Basic test program with example usage
├── advanced_test.c     # Advanced test program with image loading support
├── Makefile            # Build configuration with SDL2 support
└── README.md           # This file
```

## Data Structures

### Image
```c
typedef struct {
    SDL_Surface *surface;   // SDL2 surface for image data
    int width;              // Image width in pixels
    int height;             // Image height in pixels
    int channels;           // Number of color channels (1=grayscale, 3=RGB, 4=RGBA)
} Image;
```

### Grid
```c
typedef struct {
    Rectangle bounds;       // Grid boundary rectangle
    int rows, cols;         // Grid dimensions
    char **letters;         // 2D array of detected letters
    Point **cell_centers;   // 2D array of cell center coordinates
} Grid;
```

## Key Functions

### Main Detection Function
```c
Grid* detect_wordsearch_grid(Image* img);
```
This is the main function that processes an image and returns a detected grid structure.

### Image Processing Functions
- `convert_to_grayscale()` - Converts RGB images to grayscale
- `apply_gaussian_blur()` - Applies Gaussian blur for noise reduction
- `apply_edge_detection()` - Uses Sobel operators for edge detection
- `apply_threshold()` - Applies binary thresholding

### Grid Detection Functions
- `detect_grid_boundaries()` - Finds the rectangular boundary of the grid
- `detect_grid_dimensions()` - Determines number of rows and columns
- `extract_grid()` - Extracts individual cells and their properties

## Building and Running

### Prerequisites
- GCC compiler
- SDL2 development libraries
- Math library (libm)
- SDL2_image (optional, for image loading/saving)

### Compilation
```bash
make
```

### Running the Main Program
```bash
# Build everything
make

# Run main detection tool
./detect_grid your_image.jpg

# Show usage help
./detect_grid
```

### Running Tests
```bash
# Basic test
make test

# Advanced test with image loading support
make test-advanced
```

### Debug Build
```bash
make debug
```

### Clean Build Artifacts
```bash
make clean
```

## Quick Start

### Using the Main Program

The easiest way to use the detection system is with the main program:

```bash
# Build the project
make

# Detect grid in an image
./detect_grid wordsearch.jpg

# Show help
./detect_grid
```

**Output Example:**
```
Word Search Grid Detection Tool
===============================
Processing image: wordsearch.jpg

Initializing SDL2...
✓ SDL2 initialized successfully

Loading image...
✓ Image loaded successfully
  Dimensions: 800 × 600 pixels
  Channels: 3

Detecting word search grid...
✓ Grid detected successfully!

=== DETECTION RESULTS ===
Grid Dimensions: 10 rows × 10 columns
Grid Boundaries:
  Top-left:     (100, 150)
  Top-right:    (700, 150)
  Bottom-left:  (100, 550)
  Bottom-right: (700, 550)
  Width:        600 pixels
  Height:       400 pixels

Grid Letters:
    0  1  2  3  4  5  6  7  8  9 
 0  A  B  C  D  E  F  G  H  I  J 
 1  K  L  M  N  O  P  Q  R  S  T 
 2  U  V  W  X  Y  Z  A  B  C  D 
  ...
```

### Programmatic Usage

```c
#include "detection.h"

int main() {
    // Initialize SDL2
    if (!init_sdl()) {
        printf("Failed to initialize SDL2!\n");
        return 1;
    }
    
    // Load your image
    Image* img = load_image("wordsearch.jpg");
    
    // Detect the grid
    Grid* grid = detect_wordsearch_grid(img);
    
    if (grid) {
        // Access grid properties
        printf("Grid size: %dx%d\n", grid->rows, grid->cols);
        printf("Grid bounds: (%d,%d) to (%d,%d)\n", 
               grid->bounds.top_left.x, grid->bounds.top_left.y,
               grid->bounds.bottom_right.x, grid->bounds.bottom_right.y);
        
        // Access individual cells
        for (int row = 0; row < grid->rows; row++) {
            for (int col = 0; col < grid->cols; col++) {
                char letter = grid->letters[row][col];
                Point center = grid->cell_centers[row][col];
                printf("Cell (%d,%d): letter='%c', center=(%d,%d)\n", 
                       row, col, letter, center.x, center.y);
            }
        }
        
        // Clean up
        free_grid(grid);
    }
    
    // Clean up
    free_image(img);
    cleanup_sdl();
    return 0;
}
```

## Algorithm Overview

1. **Image Preprocessing**:
   - Convert to grayscale
   - Apply Gaussian blur to reduce noise
   - Detect edges using Sobel operators
   - Apply binary thresholding

2. **Grid Detection**:
   - Scan for edge pixels to find potential boundaries
   - Determine the largest rectangular region
   - Calculate grid dimensions by counting lines

3. **Cell Extraction**:
   - Divide the grid into equal-sized cells
   - Calculate center coordinates for each cell
   - Extract letter information (placeholder for OCR)

## Limitations and Improvements

### Current Limitations
- Simplified grid detection algorithm
- Placeholder letter detection (no actual OCR)
- Basic image loading (no real image format support)
- Assumes square grid cells

### Potential Improvements
- Integrate with OCR libraries (Tesseract, OpenCV)
- Support for various image formats (JPEG, PNG, etc.)
- More robust grid detection using Hough transforms
- Support for non-square grids
- Better handling of rotated or skewed grids
- Machine learning-based letter recognition

## Dependencies

### Required
- **SDL2**: Core graphics and surface management
- **Math library (libm)**: For mathematical operations

### Optional (for full functionality)
- **SDL2_image**: For loading/saving various image formats (PNG, JPEG, etc.)
- **OpenCV**: For advanced image processing
- **Tesseract OCR**: For actual letter recognition

## License

This project is part of an OCR coursework assignment.

## Contributing

This is an academic project. For improvements or bug fixes, please follow the existing code style and add appropriate comments.
