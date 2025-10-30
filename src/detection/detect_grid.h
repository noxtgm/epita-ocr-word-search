#ifndef DETECT_GRID_H
#define DETECT_GRID_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>

// Image structure using GTK/GdkPixbuf
typedef struct {
    GdkPixbuf *pixbuf;
    gint width;
    gint height;
    gint channels;
    guchar *cached_pixels;  // Cached pixel data for fast access
    gboolean cache_valid;   // Cache validity flag
} Image;

// Point structure for coordinates

typedef struct {
    int x, y;
} Point;
#define POINT_TYPE_DEFINED


// Grouped projection result
typedef struct {
    int* v_peaks;
    int  v_count;
    int* h_peaks;
    int  h_count;
} GridLines;

// Rectangle structure for grid boundaries
typedef struct {
    Point top_left;
    Point top_right;
    Point bottom_left;
    Point bottom_right;
    int width, height;
} Rectangle;

typedef struct {
    Point* points;
    int count;
    int v_lines;  // Number of vertical lines
    int h_lines;  // Number of horizontal lines
} IntersectionList;

typedef struct {
    Image* image;
    int row; // row index
    int col; // col index
    Rectangle bounding_box;
    char* image_file; // path of saved cell image
} GridCell;

typedef struct {
    GridCell* cells;
    int rows;
    int cols;
    int count;
} GridCells;

// Grid structure
typedef struct {
    Rectangle bounds;
    int rows, cols;
    char **letters;
    GridCell **cells; // Extracted cells as a 2D array [rows][cols]
} Grid;

// GTK initialization and cleanup
gboolean init_gtk(int *argc, char ***argv);

// Image I/O functions
Image* load_image(const char* filename);
void free_image(Image* img);
int save_image(const char* filename, Image* img);
Image* create_image(int width, int height, int channels);

// Image processing functions
Image* convert_to_grayscale(Image* img);
Image* gaussian_blur(Image* img, int kernel_size);
Image* canny_edge_detection(Image* img, int low_threshold, int high_threshold);

// Contour detection structures and functions for fall back if the line intersections not working 
typedef struct {
    Point *points;
    int count;
    int capacity;
} Contour;

typedef struct {
    Contour *contours;
    int count;
    int capacity;
} ContourList;

ContourList* find_contours(Image* img);
Rectangle find_largest_square(ContourList* contour_list);

// Pixel cache management functions
void cache_pixel_data(Image* img);
void free_pixel_cache(Image* img);
guchar get_cached_pixel(Image* img, gint x, gint y, gint channel);
void set_cached_pixel(Image* img, gint x, gint y, gint channel, guchar value);
void sync_cache_to_pixbuf(Image* img);

// Morphology (single-channel images expected)
Image* morph_erode(Image* img, int kernel_w, int kernel_h);
Image* morph_dilate(Image* img, int kernel_w, int kernel_h);
Image* morph_open(Image* img, int kernel_w, int kernel_h);
Image* open_vertical_lines(Image* img, int kernel_height, int thickness_restore);
Image* open_horizontal_lines(Image* img, int kernel_width, int thickness_restore);

// Projections and peak detection
int find_peaks(const int* values, int n, int min_spacing, int threshold, int* out_indices, int max_out);
int profile_auto_threshold(const int* values, int n); 

// Cells extract
GridCells* extract_grid_cells(Image* original_img, IntersectionList* intersections);
int save_grid_cells(GridCells* grid_cells);
Image* draw_intersections(Image* img, IntersectionList* intersections);
void free_grid_cells(GridCells* grid_cells);
Rectangle compute_grid_bounds_from_peaks(int* v_peaks, int v_count, int* h_peaks, int h_count, int img_w, int img_h);

// Compute grid lines from vertical/horizontal line images using projections
GridLines compute_grid_lines(Image* vert_lines, Image* hori_lines, int kernel_w, int kernel_h);

// Main detection function
Grid* detect_wordsearch_grid(Image* img);

// Memory management functions
void free_grid(Grid* grid);

#endif
