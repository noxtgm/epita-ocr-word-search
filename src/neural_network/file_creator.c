#include "file_creator.h"

// Create directory recursively using mkdir -p
int ensure_directory_exists(const char *path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        // Directory doesn't exist, try to create it
        if (mkdir(path, 0755) == -1) {
            // Try creating parent directories
            char cmd[MAX_PATH];
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
            return system(cmd);
        }
    }
    return 0;
}

// Extract row and column from filename like "cell_3_7.png"
int parse_cell_filename(const char *filename, int *row, int *col) {
    if (strncmp(filename, "cells_", 6) != 0) return 0;
    
    const char *ptr = filename + 6;
    char *end;
    
    *row = strtol(ptr, &end, 10);
    if (*end != '_') return 0;
    
    ptr = end + 1;
    *col = strtol(ptr, &end, 10);
    if (strcmp(end, ".png") != 0) return 0;
    
    return 1;
}

// Sort cells by row first, then by column (for proper grid ordering)
int compare_cells(const void *a, const void *b) {
    CellResult *ca = (CellResult*)a;
    CellResult *cb = (CellResult*)b;
    
    if (ca->row != cb->row) return ca->row - cb->row;
    return ca->col - cb->col;
}

// Use shell command to list PNG files in directory
int get_png_files(const char *dir, char filenames[][256], int max_files) {
    char cmd[MAX_PATH];
    snprintf(cmd, sizeof(cmd), "ls %s/*.png 2>/dev/null", dir);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    
    int count = 0;
    char path[MAX_PATH];
    
    while (fgets(path, sizeof(path), fp) && count < max_files) {
        path[strcspn(path, "\n")] = 0;  // Remove newline
        
        // Extract just the filename from full path
        char *filename = strrchr(path, '/');
        if (filename) {
            strncpy(filenames[count], filename + 1, 255);
            filenames[count][255] = '\0';
            count++;
        }
    }
    
    pclose(fp);
    return count;
}

// Process all grid cells: load, recognize, and store results
int process_grid(MLP *model, const char *cell_dir, CellResult *results) {
    char (*filenames)[256] = malloc(sizeof(char[256]) * MAX_FILES);
    int file_count = get_png_files(cell_dir, filenames, MAX_FILES);
    
    if (file_count == 0) {
        fprintf(stderr, "No PNG files found in: %s\n", cell_dir);
        free(filenames);
        return 0;
    }
    
    printf("Found %d PNG files in grid directory\n", file_count);
    
    int count = 0;
    double *output = malloc(sizeof(double) * model->output_size);
    
    for (int f = 0; f < file_count; f++) {
        int row, col;
        if (!parse_cell_filename(filenames[f], &row, &col)) continue;
        
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", cell_dir, filenames[f]);
        
        // Load and preprocess image
        double *img = load_image_enhanced(path);
        if (!img) {
            fprintf(stderr, "Failed to load: %s\n", path);
            continue;
        }
        
        // Run neural network prediction
        mlp_predict(model, img, output);
        free(img);
        
        // Find predicted letter (highest probability)
        int max_idx = 0;
        for (int i = 1; i < model->output_size; i++) {
            if (output[i] > output[max_idx]) max_idx = i;
        }
        
        // Store result with position and confidence
        results[count].row = row;
        results[count].col = col;
        results[count].letter = 'A' + max_idx;
        results[count].confidence = output[max_idx];
        count++;
        
        if (count % 50 == 0) {
            printf("Processed %d cells...\n", count);
        }
    }
    
    free(output);
    free(filenames);
    
    return count;
}

// Enhanced image preprocessing (same as training)
double* load_image_enhanced(const char *path) {
    GError *error = NULL;
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &error);
    if (!pixbuf) {
        fprintf(stderr, "Failed to load %s: %s\n", path, error->message);
        g_error_free(error);
        return NULL;
    }
    
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, TARGET_WIDTH, TARGET_HEIGHT, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    if (!scaled) return NULL;
    
    int n_channels = gdk_pixbuf_get_n_channels(scaled);
    int rowstride = gdk_pixbuf_get_rowstride(scaled);
    guchar *pixels = gdk_pixbuf_get_pixels(scaled);
    
    double *gray = malloc(sizeof(double) * INPUT_SIZE);
    double *normalized = malloc(sizeof(double) * INPUT_SIZE);
    
    // Convert to grayscale
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            guchar *pixel = pixels + y * rowstride + x * n_channels;
            if (n_channels >= 3) {
                gray[y * TARGET_WIDTH + x] = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
            } else {
                gray[y * TARGET_WIDTH + x] = pixel[0];
            }
        }
    }
    
    // Adaptive thresholding (same as training)
    int window = 5;
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            double sum = 0.0;
            int count = 0;
            
            for (int dy = -window; dy <= window; dy++) {
                for (int dx = -window; dx <= window; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (ny >= 0 && ny < TARGET_HEIGHT && nx >= 0 && nx < TARGET_WIDTH) {
                        sum += gray[ny * TARGET_WIDTH + nx];
                        count++;
                    }
                }
            }
            
            double local_mean = sum / count;
            double threshold = local_mean * 0.9;
            normalized[y * TARGET_WIDTH + x] = (gray[y * TARGET_WIDTH + x] < threshold) ? 0.0 : 1.0;
        }
    }
    
    // Normalize
    double mean = 0.0;
    for (int i = 0; i < INPUT_SIZE; i++) mean += normalized[i];
    mean /= INPUT_SIZE;
    
    for (int i = 0; i < INPUT_SIZE; i++) {
        normalized[i] = (normalized[i] - mean) / 0.5;
    }
    
    free(gray);
    g_object_unref(scaled);
    return normalized;
}

// Process word directories: recognize letters in each word
int process_words(MLP *model, const char *word_dir_pattern, char words[][50]) {
    int word_count = 0;
    
    // Try word_0, word_1, word_2, etc.
    for (int w = 0; w < MAX_WORDS; w++) {
        char word_dir[MAX_PATH];
        snprintf(word_dir, sizeof(word_dir), word_dir_pattern, w);
        
        // Check if directory exists
        char test_cmd[MAX_PATH + 10];
        snprintf(test_cmd, sizeof(test_cmd), "test -d %s", word_dir);
        if (system(test_cmd) != 0) break;  // No more word directories
        
        char (*filenames)[256] = malloc(sizeof(char[256]) * 100);
        int file_count = get_png_files(word_dir, filenames, 100);
        
        if (file_count == 0) {
            free(filenames);
            continue;
        }
        
        // Collect and sort character cells by index
        typedef struct {
            char filename[256];
            int index;
        } WordCell;
        
        WordCell *cells = malloc(sizeof(WordCell) * file_count);
        int cell_count = 0;
        
        for (int f = 0; f < file_count; f++) {
            // Extract index from filename (e.g., "char_2.png" -> 2)
            int idx = 0;
            if (sscanf(filenames[f], "char_%d.png", &idx) != 1) {
                // Try old format as fallback
                sscanf(filenames[f], "cells_%d.png", &idx);
            }
            
            strncpy(cells[cell_count].filename, filenames[f], 255);
            cells[cell_count].index = idx;
            cell_count++;
        }
        
        free(filenames);
        
        // Sort by index to get correct letter order
        for (int i = 0; i < cell_count - 1; i++) {
            for (int j = i + 1; j < cell_count; j++) {
                if (cells[i].index > cells[j].index) {
                    WordCell temp = cells[i];
                    cells[i] = cells[j];
                    cells[j] = temp;
                }
            }
        }
        
        // Recognize each letter and build word
        double *output = malloc(sizeof(double) * model->output_size);
        char word[100] = "";
        
        for (int c = 0; c < cell_count; c++) {
            char path[MAX_PATH * 2];
            snprintf(path, sizeof(path), "%s/%s", word_dir, cells[c].filename);
            
            double *img = load_image_enhanced(path);
            if (!img) continue;
            
            mlp_predict(model, img, output);
            free(img);
            
            // Find predicted letter
            int max_idx = 0;
            for (int i = 1; i < model->output_size; i++) {
                if (output[i] > output[max_idx]) max_idx = i;
            }
            
            char letter = 'A' + max_idx;
            strncat(word, &letter, 1);
        }
        
        free(output);
        free(cells);
        
        // Store recognized word
        if (strlen(word) > 0) {
            size_t len = strlen(word);
            if (len > 49) len = 49;
            memcpy(words[word_count], word, len);
            words[word_count][len] = '\0';
            printf("  Word %d: %s\n", w, word);
            word_count++;
        }
    }
    
    return word_count;
}

// Write 2D grid of recognized letters to file
void write_grid(const char *filename, CellResult *results, int count) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Cannot create grid file");
        return;
    }
    
    // Find grid dimensions
    int max_row = 0, max_col = 0;
    for (int i = 0; i < count; i++) {
        if (results[i].row > max_row) max_row = results[i].row;
        if (results[i].col > max_col) max_col = results[i].col;
    }
    
    // Create 2D array for the grid
    char grid[MAX_ROWS][MAX_COLS];
    for (int r = 0; r <= max_row; r++) {
        for (int c = 0; c <= max_col; c++) {
            grid[r][c] = '?';  // Default for unrecognized cells
        }
    }
    
    // Fill in recognized letters
    for (int i = 0; i < count; i++) {
        grid[results[i].row][results[i].col] = results[i].letter;
    }
    
    // Write grid to file
    for (int r = 0; r <= max_row; r++) {
        for (int c = 0; c <= max_col; c++) {
            fprintf(f, "%c", grid[r][c]);
        }
        fprintf(f, "\n");
    }
    
    fclose(f);
    printf("\nGrid saved to: %s\n", filename);
}

// Write list of recognized words to file
void write_wordlist(const char *filename, char words[][50], int count) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Cannot create wordlist file");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(f, "%s\n", words[i]);
    }
    
    fclose(f);
    printf("Word list saved to: %s\n", filename);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <model.bin> [grid_output.txt] [wordlist_output.txt]\n", argv[0]);
        printf("\nRecognizes letters in grid cells and word images.\n");
        printf("\nExpected directory structure:\n");
        printf("  ../../outputs/grid_detection/cells/cells_R_C.png\n");
        printf("  ../../outputs/list_detection/word_N/char_XXX.png\n");
        printf("\nOutput files will be saved to: %s/\n", OUTPUT_DIR);
        return 1;
    }
    
    const char *model_path = argv[1];
    
    // Construct full output paths
    char grid_output_path[MAX_PATH];
    char wordlist_output_path[MAX_PATH];
    
    if (argc > 2) {
        // If user provides custom filename, still put it in OUTPUT_DIR
        const char *grid_filename = strrchr(argv[2], '/');
        if (grid_filename) {
            grid_filename++;
        } else {
            grid_filename = argv[2];
        }
        snprintf(grid_output_path, sizeof(grid_output_path), "%s/%s", OUTPUT_DIR, grid_filename);
    } else {
        snprintf(grid_output_path, sizeof(grid_output_path), "%s/recognized_grid.txt", OUTPUT_DIR);
    }
    
    if (argc > 3) {
        const char *wordlist_filename = strrchr(argv[3], '/');
        if (wordlist_filename) {
            wordlist_filename++;
        } else {
            wordlist_filename = argv[3];
        }
        snprintf(wordlist_output_path, sizeof(wordlist_output_path), "%s/%s", OUTPUT_DIR, wordlist_filename);
    } else {
        snprintf(wordlist_output_path, sizeof(wordlist_output_path), "%s/recognized_words.txt", OUTPUT_DIR);
    }
    
    // Ensure output directory exists
    printf("Ensuring output directory exists: %s\n", OUTPUT_DIR);
    if (ensure_directory_exists(OUTPUT_DIR) != 0) {
        fprintf(stderr, "Warning: Could not create output directory\n");
    }
    
    // Load trained model
    printf("Loading model from: %s\n", model_path);
    MLP *model = mlp_load(model_path);
    if (!model) {
        fprintf(stderr, "Failed to load model!\n");
        return 1;
    }
    
    printf("Model loaded successfully.\n");
    printf("  Input: %d, Hidden: %d, Output: %d\n\n", 
           model->input_size, model->hidden_size, model->output_size);
    
    // Process grid cells
    printf("=== Processing Grid Cells ===\n");
    CellResult *grid_results = malloc(sizeof(CellResult) * MAX_FILES);
    int grid_count = process_grid(model, "../../outputs/grid_detection/cells", grid_results);
    
    if (grid_count == 0) {
        fprintf(stderr, "No grid cells found!\n");
        free(grid_results);
        mlp_free(model);
        return 1;
    }
    
    printf("Found %d grid cells.\n", grid_count);
    
    // Sort results by position for proper grid layout
    qsort(grid_results, grid_count, sizeof(CellResult), compare_cells);
    
    // Write grid to file
    write_grid(grid_output_path, grid_results, grid_count);
    
    // Process word images
    printf("\n=== Processing Word Lists ===\n");
    char words[MAX_WORDS][50];
    int word_count = process_words(model, "../../outputs/list_detection/word_%d", words);
    
    if (word_count > 0) {
        printf("\nFound %d words.\n", word_count);
        write_wordlist(wordlist_output_path, words, word_count);
    } else {
        printf("No word directories found.\n");
    }
    
    // Print summary
    printf("\n=== Summary ===\n");
    printf("Grid cells processed: %d\n", grid_count);
    printf("Words recognized: %d\n", word_count);
    printf("\nOutput files:\n");
    printf("  Grid: %s\n", grid_output_path);
    if (word_count > 0) {
        printf("  Words: %s\n", wordlist_output_path);
    }
    
    // Cleanup
    free(grid_results);
    mlp_free(model);
    
    return 0;
}
