#include "detect.h"

void print_grid_info(Grid* grid) {
    if (!grid) {
        printf("No grid information available.\n");
        return;
    }
    
    printf("Grid dimensions: %d rows × %d columns\n", grid->rows, grid->cols);
    printf("Grid boundaries:\n");
    printf("  Top-left:     (%d, %d)\n", grid->bounds.top_left.x, grid->bounds.top_left.y);
    printf("  Top-right:    (%d, %d)\n", grid->bounds.top_right.x, grid->bounds.top_right.y);
    printf("  Bottom-left:  (%d, %d)\n", grid->bounds.bottom_left.x, grid->bounds.bottom_left.y);
    printf("  Bottom-right: (%d, %d)\n", grid->bounds.bottom_right.x, grid->bounds.bottom_right.y);
    printf("  Width:        %d pixels\n", grid->bounds.width);
    printf("  Height:       %d pixels\n", grid->bounds.height);
}

int main(int argc, char* argv[]) {
    if (!init_gtk(&argc, &argv)) {
        printf("ERROR: Failed to initialize GTK!\n");
        return 1;
    }
    
    Grid *grid;
    detect(argc,argv,&grid);
    
    if (grid) {
        printf("Grid detected successfully\n");
        print_grid_info(grid);
        // Clean up grid
        free_grid(grid);
    } else {
        printf("No grid detected in the image\n");
        printf("\nPossible reason:\n");
        printf("- Image doesn't contain a clear grid pattern\n");
    }
    
    return (grid != NULL) ? 0 : 1;
}
