#ifndef SOLVER_H
#define SOLVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_GRID_SIZE 100 // Maximum size of the grid
#define MAX_LINE_LENGTH 256 // Maximum length of a line, also the size of the buffer when reading the grid file

typedef struct {
    char grid[MAX_GRID_SIZE][MAX_GRID_SIZE]; // Grid data
    int rows; // Number of rows
    int cols; // Number of columns
} Grid;

typedef struct {
    int x; // Row coordinate
    int y; // Column coordinate
} Position;

// Load the grid from a file
int load_grid(const char *file_name, Grid *grid);

// Check if a position is valid in the grid
int is_valid_position(const Grid *grid, int x, int y);

// Search for a word in a specific direction from a starting position
int search_word_in_direction(const Grid *grid, const char *word, int start_x,
                             int start_y, int dir, Position *end_pos);

// Search for a word in the entire grid
int search_word(const Grid *grid, const char *word, Position *start_pos,
                Position *end_pos);

// Solve the word search
int solve_word_search(const char *file_name, const char *word);

#endif
