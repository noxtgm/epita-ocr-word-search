#include "detect_grid.h"
#include "detect_list.h"

void print_usage(const char* program_name) {
    printf("Word Search Grid Detection Tool\n");
    printf("===============================\n");
    printf("Usage: %s <image_file>\n", program_name);
    printf("\n");
    printf("This program detects word search grids in images and outputs:\n");
    printf("- Grid dimensions (rows x columns)\n");
    printf("- Grid boundaries (coordinates)\n");
    printf("- Cell boundaries\n");
    printf("\n");
}

void print_grid_info(Grid* grid) {
    if (!grid) {
        printf("No grid information available.\n");
        return;
    }
    
    printf("\n=== DETECTION RESULTS ===\n");
    printf("Grid Dimensions: %d rows × %d columns\n", grid->rows, grid->cols);
    printf("Grid Boundaries:\n");
    printf("  Top-left:     (%d, %d)\n", grid->bounds.top_left.x, grid->bounds.top_left.y);
    printf("  Top-right:    (%d, %d)\n", grid->bounds.top_right.x, grid->bounds.top_right.y);
    printf("  Bottom-left:  (%d, %d)\n", grid->bounds.bottom_left.x, grid->bounds.bottom_left.y);
    printf("  Bottom-right: (%d, %d)\n", grid->bounds.bottom_right.x, grid->bounds.bottom_right.y);
    printf("  Width:        %d pixels\n", grid->bounds.width);
    printf("  Height:       %d pixels\n", grid->bounds.height);
}

int main(int argc, char* argv[]) {
    // Initialize GTK first (before command line processing)
    printf("Word Search Grid Detection Tool\n");
    printf("===============================\n");
    printf("\nInitializing GTK...\n");
    if (!init_gtk(&argc, &argv)) {
        printf("ERROR: Failed to initialize GTK!\n");
        return 1;
    }
    printf("GTK initialized successfully\n");
    
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* image_file = argv[1];
    printf("Processing image: %s\n", image_file);
    
    // Load the image
    printf("\nLoading image...\n");
    Image* img = load_image(image_file);
    if (!img) {
        printf("ERROR: Failed to load image '%s'!\n", image_file);
        printf("Supported formats: PNG, JPEG, GIF, BMP, TIFF\n");
        return 1;
    }
    printf("  Image loaded successfully\n");
    printf("  Dimensions: %d × %d pixels\n", img->width, img->height);
    printf("  Channels: %d\n", img->channels);
    
 
    
    // Detect the word search grid
    printf("\nDetecting word search grid...\n");
    
    Grid* grid = detect_wordsearch_grid(img);
    
    if (grid) {
        printf("Grid detected successfully!\n");
        print_grid_info(grid);
        
        // Clean up grid
        free_grid(grid);
        printf("Grid memory freed\n");
    } else {
        printf("No grid detected in the image\n");
        printf("\nPossible reason:\n");
        printf("- Image doesn't contain a clear grid pattern\n");
    }
    
    // Clean up
    printf("\nCleaning up...\n");
    free_image(img);
    printf("Cleanup completed\n");
    
    printf("\n=== Detection completed ===\n");
    return (grid != NULL) ? 0 : 1;
}
