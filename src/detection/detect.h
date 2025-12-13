#ifndef DETECT_H
#define DETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <gtk/gtk.h>
#include <glib.h>

// Platform-specific mkdir declarations
#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path, mode) mkdir(path, mode)
#endif

// Point structure for coordinates
typedef struct {
    int x, y;
} Point;

// Rectangle structure for grid boundaries
typedef struct {
    Point top_left;
    Point top_right;
    Point bottom_left;
    Point bottom_right;
    int width, height;
} Rectangle;

// Box structure for detected components
typedef struct {
    int minx, maxx, miny, maxy;
    int pixel_count;
} Box;

// Character info structure
typedef struct {
    int char_index;
    int minx, maxx, miny, maxy;
    int width, height;
    int pixel_count;
    int center_x, center_y;
} CharInfo;

// Grid cell structure
typedef struct {
    int row;
    int col;
    Rectangle bounding_box;
    char* image_file;
} GridCell;

// Grid structure
typedef struct {
    Rectangle bounds;
    int rows, cols;
    char **chars;
    GridCell **cells;
} Grid;

// Row data structure
typedef struct {
    int** row_chars;
    int* row_counts;
    int num_rows;
    float* row_y;
} RowData;

// Image structure using GTK/GdkPixbuf
typedef struct {
    GdkPixbuf *pixbuf;
    gint width;
    gint height;
    gint channels;
    guchar *cached_pixels;
    gboolean cache_valid;
} Image;

typedef struct {
    Box* boxes;
    int box_count;
    unsigned char** binary;
    int** visited;
    Image* work_img;
} DetectionData;

// GTK initialization
gboolean init_gtk(int *argc, char ***argv);

// Image I/O functions
Image* load_image(const char* filename);
void free_image(Image* img);
int save_image(const char* filename, Image* img);
Image* create_image(int width, int height, int channels);

// Pixel management functions
void cache_pixel_data(Image* img);
void free_pixel_cache(Image* img);
guchar get_cached_pixel(Image* img, gint x, gint y, gint channel);
void set_cached_pixel(Image* img, gint x, gint y, gint channel, guchar value);
void sync_cache_to_pixbuf(Image* img);

// Image processing functions
void enhance_contrast(Image* img);
void convert_to_grayscale(Image* img);
int detect_components(DetectionData* data, Image* img, 
    int x1, int y1, int x2, int y2,
    int min_width, int min_height,
    int min_pixels, int* box_capacity);
void adaptive_threshold(Image* img, unsigned char **binary, int block_size);


// Flood fill for connected components
void flood_fill(unsigned char **binary, int **visited,
                int width, int height, int sx, int sy,
                int *minx, int *maxx, int *miny, int *maxy, int *pixel_count,
                int x1, int y1, int x2, int y2);

// Drawing functions
void save_grid_debug_image(Image* img, Grid* grid, const char* output_path);
void draw_rect(Image *img, int x1, int y1, int x2, int y2);
void draw_rect_green(Image *img, int x1, int y1, int x2, int y2);

// Utility functions
int FxOGrA(const char *name, const char *a, const char *b);
void remove_directory(const char* path);

// Grid detection
CharInfo* filter_grid_characters(CharInfo* chars, int char_count, int* filtered_count, 
    float spacing_tolerance, int*** out_row_chars, int** out_row_counts, 
    int* out_num_rows, float** out_row_y);

int detect(int argc, char **argv, Grid **grid);
Grid* detect_grid(Image* img, DetectionData* data);
int detect_list(Image *img, DetectionData* data, int x1, int y1, int x2, int y2, 
    const char* filename);
DetectionData* preprocess_image(Image* img, int threshold);


// Memory management
void free_grid(Grid* grid);
void free_detection_data(DetectionData* data, int height);

// List detection main
int detect_list_main(int argc, char **argv);

#endif
