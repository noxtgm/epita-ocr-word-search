#include "neuralnetwork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// Enhanced image loading (uses adaptive thresholding)
double* load_image_enhanced(const char *path);

#define MAX_ROWS 50
#define MAX_COLS 50
#define MAX_WORDS 100
#define MAX_FILES 10000
#define MAX_PATH 2048

// Output directory for recognized files
#define OUTPUT_DIR "../../outputs/recognized_files"

// Structure to store recognized cell with position and confidence
typedef struct {
    int row;
    int col;
    char letter;
    double confidence;
} CellResult;

// Create directory if it doesn't exist (with parent directories)
int ensure_directory_exists(const char *path);

// Parse cell filename like "cell_0_0.png" to extract row and column indices
int parse_cell_filename(const char *filename, int *row, int *col);

// Comparison function for sorting cells by position (row-major order)
int compare_cells(const void *a, const void *b);

// Get list of PNG files in directory using shell command
int get_png_files(const char *dir, char filenames[][256], int max_files);

// Process grid cells: recognize each cell and store result with position
int process_grid(MLP *model, const char *cell_dir, CellResult *results);

// Process word images: recognize letters in each word directory
int process_words(MLP *model, const char *word_dir_pattern, char words[][50]);

// Write recognized grid to text file in 2D format
void write_grid(const char *filename, CellResult *results, int count);

// Write recognized words to text file (one word per line)
void write_wordlist(const char *filename, char words[][50], int count);

int main(int argc, char **argv);
