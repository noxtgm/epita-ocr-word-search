#ifndef DETECTION_H
#define DETECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

// Image structure using SDL2
typedef struct {
    SDL_Surface *surface;
    int width;
    int height;
    int channels;
} Image;

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

// Grid structure
typedef struct {
    Rectangle bounds;
    int rows, cols;
    char **letters;
    Point **cell_centers;
} Grid;

// Function declarations

// SDL2 initialization and cleanup
int init_sdl();
void cleanup_sdl();

// Image I/O functions
Image* load_image(const char* filename);
void free_image(Image* img);
int save_image(const char* filename, Image* img);
Image* create_image(int width, int height, int channels);

// Image processing functions
Image* convert_to_grayscale(Image* img);
Image* apply_gaussian_blur(Image* img, int kernel_size);
Image* apply_edge_detection(Image* img);
Image* apply_threshold(Image* img, int threshold);

// SDL2 utility functions
Uint32 get_pixel(SDL_Surface *surface, int x, int y);
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
SDL_Surface* create_surface(int width, int height, Uint32 format);

// Grid detection functions
Rectangle detect_grid_boundaries(Image* img);
Grid* extract_grid(Image* img, Rectangle bounds);
Point* find_corner_points(Image* img, Rectangle* rect);
int detect_grid_dimensions(Image* img, Rectangle bounds);

// Letter detection functions
char detect_letter_in_cell(Image* img, Point center, int cell_width, int cell_height);
char* preprocess_letter_image(Image* img, Rectangle cell_rect);

// Utility functions
double calculate_distance(Point p1, Point p2);
Point calculate_center(Point p1, Point p2);
int is_valid_grid_cell(Image* img, Point center, int cell_width, int cell_height);

// Main detection function
Grid* detect_wordsearch_grid(Image* img);

// Memory management functions
void free_grid(Grid* grid);

#endif
