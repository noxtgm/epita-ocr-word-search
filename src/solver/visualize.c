#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

// Draw a red circle around a grid cell
void draw_circle_on_cell(GdkPixbuf *pixbuf, int col, int row, int cell_width, int cell_height, int thickness) {
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    
    int cx = col * cell_width + cell_width / 2;
    int cy = row * cell_height + cell_height / 2;
    int radius = (cell_width < cell_height ? cell_width : cell_height) / 2 + thickness;
    
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    
    // Draw circle using midpoint circle algorithm
    for (int t = 0; t < thickness; t++) {
        int r = radius - t;
        int x = 0;
        int y = r;
        int d = 3 - 2 * r;
        
        while (y >= x) {
            // Draw 8 octants
            int points[8][2] = {
                {cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y},
                {cx + y, cy + x}, {cx - y, cy + x}, {cx + y, cy - x}, {cx - y, cy - x}
            };
            
            for (int i = 0; i < 8; i++) {
                int px = points[i][0];
                int py = points[i][1];
                
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    guchar *p = pixels + py * rowstride + px * n_channels;
                    p[0] = 255;  // Red
                    p[1] = 0;    // Green
                    p[2] = 0;    // Blue
                }
            }
            
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <grid_image> <positions_file> <output_image>\n", argv[0]);
        return 1;
    }
    
    const char *image_path = argv[1];
    const char *positions_path = argv[2];
    const char *output_path = argv[3];
    
    // Load the grid image
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(image_path, &error);
    if (!pixbuf) {
        fprintf(stderr, "Error loading image: %s\n", error ? error->message : "unknown");
        if (error) g_error_free(error);
        return 1;
    }
    
    // Open positions file
    FILE *fp = fopen(positions_path, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open positions file\n");
        g_object_unref(pixbuf);
        return 1;
    }
    
    // Read grid dimensions from the first word's coordinates to estimate cell size
    int img_width = gdk_pixbuf_get_width(pixbuf);
    int img_height = gdk_pixbuf_get_height(pixbuf);
    
    // Parse positions file and count max grid dimensions
    char line[512];
    int max_col = 0, max_row = 0;
    
    // First pass: find grid dimensions
    while (fgets(line, sizeof(line), fp)) {
        char word[100];
        int y1, x1, y2, x2;
        if (sscanf(line, "%s (%d,%d)(%d,%d)", word, &y1, &x1, &y2, &x2) == 5) {
            if (y1 > max_col) max_col = y1;
            if (y2 > max_col) max_col = y2;
            if (x1 > max_row) max_row = x1;
            if (x2 > max_row) max_row = x2;
        }
    }
    
    // Estimate cell dimensions
    int grid_cols = max_col + 1;
    int grid_rows = max_row + 1;
    int cell_width = img_width / grid_cols;
    int cell_height = img_height / grid_rows;
    
    printf("Grid dimensions: %dx%d cells\n", grid_cols, grid_rows);
    printf("Cell size: %dx%d pixels\n", cell_width, cell_height);
    
    // Second pass: draw circles
    rewind(fp);
    while (fgets(line, sizeof(line), fp)) {
        char word[100];
        int y1, x1, y2, x2;
        if (sscanf(line, "%s (%d,%d)(%d,%d)", word, &y1, &x1, &y2, &x2) == 5) {
            printf("Drawing circles for word: %s at (%d,%d) to (%d,%d)\n", word, y1, x1, y2, x2);
            
            // Draw circle for each letter in the word
            int dx = (x2 > x1) ? 1 : ((x2 < x1) ? -1 : 0);
            int dy = (y2 > y1) ? 1 : ((y2 < y1) ? -1 : 0);
            
            int x = x1, y = y1;
            while (1) {
                draw_circle_on_cell(pixbuf, y, x, cell_width, cell_height, 3);
                
                if (x == x2 && y == y2) break;
                x += dx;
                y += dy;
            }
        }
    }
    
    fclose(fp);
    
    // Save the result
    gdk_pixbuf_save(pixbuf, output_path, "png", &error, NULL);
    if (error) {
        fprintf(stderr, "Error saving image: %s\n", error->message);
        g_error_free(error);
        g_object_unref(pixbuf);
        return 1;
    }
    
    printf("Visualization saved to: %s\n", output_path);
    g_object_unref(pixbuf);
    return 0;
}

