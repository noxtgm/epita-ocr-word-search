#include "detect.h"

void print_usage(const char* program_name) {
    printf("Word Search Grid Detection Tool\n");
    printf("===============================\n");
    printf("Usage:\n");
    printf("  Grid detection:  %s <image_file>\n", program_name);
    printf("  List detection:   %s <image_file> x1 y1 x2 y2\n", program_name);
    printf("\n");
    printf("Grid detection outputs:\n");
    printf("- Grid dimensions (rows x columns)\n");
    printf("- Grid boundaries (coordinates)\n");
    printf("- Cell boundaries\n");
    printf("\n");
    printf("List detection outputs:\n");
    printf("- Extracted characters from word lists\n");
    printf("- Organized by words/rows\n");
    printf("\n");
}

void print_grid_info(Grid* grid) {
    if (!grid) {
        printf("No grid information available.\n");
        return;
    }
    
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
    printf("Initializing GTK...\n");
    if (!init_gtk(&argc, &argv)) {
        printf("ERROR: Failed to initialize GTK!\n");
        return 1;
    }
    printf("GTK initialized successfully\n");
    // Otherwise, do grid detection (2 arguments: program, image)
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    
    // Detect the word search grid
    printf("\nDetecting word search grid...\n");
    
    Grid *grid;
    detect(argc,argv,&grid);
    
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
    printf("Cleanup completed\n");
    
    return (grid != NULL) ? 0 : 1;
}
