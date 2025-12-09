#include "label_cells.h"

// Display image as ASCII art for visual inspection during manual labeling
void show_ascii_preview(unsigned char *img, int w, int h) {
    printf("\nPreview:\n");
    int step_x = w / 40;
    int step_y = h / 20;
    if (step_x < 1) step_x = 1;
    if (step_y < 1) step_y = 1;
    
    for (int y = 0; y < h; y += step_y) {
        for (int x = 0; x < w; x += step_x) {
            int idx = y * w + x;
            unsigned char val = img[idx];
            
            if (val < 64) printf("██");
            else if (val < 128) printf("▓▓");
            else if (val < 192) printf("▒▒");
            else printf("  ");
        }
        printf("\n");
    }
}

// Compare function for qsort - sorts files naturally (e.g., char_0, char_1, ..., char_10)
int compare_files(const void *a, const void *b) {
    const char *file_a = *(const char **)a;
    const char *file_b = *(const char **)b;
    
    // Extract just the filename from full path
    const char *name_a = strrchr(file_a, '/');
    const char *name_b = strrchr(file_b, '/');
    name_a = name_a ? name_a + 1 : file_a;
    name_b = name_b ? name_b + 1 : file_b;
    
    // Try to extract numbers from filenames for natural sorting
    int num_a = -1, num_b = -1;
    
    // For char_XXX.png or cell_X_X.png patterns
    sscanf(name_a, "char_%d", &num_a);
    sscanf(name_b, "char_%d", &num_b);
    
    if (num_a == -1) sscanf(name_a, "cell_%d", &num_a);
    if (num_b == -1) sscanf(name_b, "cell_%d", &num_b);
    
    if (num_a != -1 && num_b != -1) {
        return num_a - num_b;
    }
    
    return strcmp(name_a, name_b);
}

// Recursively collect all PNG files from word_X subdirectories
int collect_image_files(const char *base_dir, char ***files, int *count, int *capacity) {
    DIR *dir = opendir(base_dir);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", base_dir);
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char subdir_path[1024];
        snprintf(subdir_path, sizeof(subdir_path), "%s/%s", base_dir, entry->d_name);
        
        struct stat st;
        if (stat(subdir_path, &st) != 0) continue;
        
        // If it's a directory (like word_0, word_1, etc.)
        if (S_ISDIR(st.st_mode)) {
            DIR *subdir = opendir(subdir_path);
            if (!subdir) continue;
            
            struct dirent *subentry;
            while ((subentry = readdir(subdir)) != NULL) {
                if (subentry->d_name[0] == '.') continue;
                
                char *ext = strrchr(subentry->d_name, '.');
                if (ext && strcmp(ext, ".png") == 0) {
                    // Add file to list
                    if (*count >= *capacity) {
                        *capacity = (*capacity == 0) ? 1000 : (*capacity * 2);
                        *files = realloc(*files, *capacity * sizeof(char*));
                        if (!*files) {
                            fprintf(stderr, "Memory allocation failed\n");
                            closedir(subdir);
                            closedir(dir);
                            return -1;
                        }
                    }
                    
                    // Store full path
                    char full_path[2048];
                    snprintf(full_path, sizeof(full_path), "%s/%s", subdir_path, subentry->d_name);
                    (*files)[*count] = strdup(full_path);
                    (*count)++;
                }
            }
            closedir(subdir);
        }
        // If it's a PNG file directly in the directory
        else {
            char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".png") == 0) {
                if (*count >= *capacity) {
                    *capacity = (*capacity == 0) ? 1000 : (*capacity * 2);
                    *files = realloc(*files, *capacity * sizeof(char*));
                    if (!*files) {
                        fprintf(stderr, "Memory allocation failed\n");
                        closedir(dir);
                        return -1;
                    }
                }
                
                (*files)[*count] = strdup(subdir_path);
                (*count)++;
            }
        }
    }
    
    closedir(dir);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_dir> <output_file>\n", argv[0]);
        printf("Examples:\n");
        printf("  %s dataset/list1 labels_list1.txt\n", argv[0]);
        printf("  %s dataset/cells1 labels_cells1.txt\n", argv[0]);
        return 1;
    }
    
    const char *input_dir = argv[1];
    const char *output_file = argv[2];
    
    // Collect all image files (handles both flat directories and word_X subdirectories)
    char **files = NULL;
    int file_count = 0;
    int file_capacity = 0;
    
    if (collect_image_files(input_dir, &files, &file_count, &file_capacity) != 0) {
        fprintf(stderr, "Failed to collect image files\n");
        return 1;
    }
    
    if (file_count == 0) {
        printf("No PNG files found in %s\n", input_dir);
        return 1;
    }
    
    // Sort files naturally (char_0, char_1, ..., char_10, char_11, etc.)
    qsort(files, file_count, sizeof(char*), compare_files);
    
    // Open output file in append mode
    FILE *out = fopen(output_file, "a");
    if (!out) {
        fprintf(stderr, "Cannot open output file: %s\n", output_file);
        for (int i = 0; i < file_count; i++) free(files[i]);
        free(files);
        return 1;
    }
    
    printf("=== Letter Labeling Tool ===\n");
    printf("Found %d PNG files in %s\n", file_count, input_dir);
    printf("\nInstructions:\n");
    printf("  - Type letter A-Z for the cell\n");
    printf("  - Type '7' to skip\n");
    printf("  - Type '8' to quit\n\n");
    
    int processed = 0, labeled = 0, skipped = 0;
    
    // Process each PNG file
    for (int i = 0; i < file_count; i++) {
        const char *img_path = files[i];
        
        // Load image in grayscale
        int width, height, channels;
        unsigned char *img = stbi_load(img_path, &width, &height, &channels, 1);
        
        if (!img) {
            fprintf(stderr, "Failed to load: %s\n", img_path);
            continue;
        }
        
        processed++;
        
        // Display image information and ASCII preview
        printf("\n[%d/%d] ================================\n", processed, file_count);
        printf("File: %s\n", img_path);
        printf("Size: %dx%d\n", width, height);
        show_ascii_preview(img, width, height);
        stbi_image_free(img);
        
        // Get user input
        printf("\nLabel (A-Z/7=skip/8=quit): ");
        fflush(stdout);
        
        char input[10];
        if (!fgets(input, sizeof(input), stdin)) break;
        
        input[strcspn(input, "\n")] = 0;
        
        char label = toupper(input[0]);
        
        // Handle quit command
        if (label == '8') {
            printf("Quitting...\n");
            break;
        }
        
        // Handle skip command
        if (label == '7') {
            skipped++;
            printf("Skipped.\n");
            continue;
        }
        
        // Validate and save label
        if (label >= 'A' && label <= 'Z') {
            // Write to output file in format: LETTER PATH
            fprintf(out, "%c %s\n", label, img_path);
            fflush(out);
            labeled++;
            printf("✓ Labeled as '%c'\n", label);
        } else {
            printf("Invalid input, skipping.\n");
        }
    }
    
    fclose(out);
    
    // Free memory
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }
    free(files);
    
    printf("\n=== Summary ===\n");
    printf("Total processed: %d\n", processed);
    printf("Labeled: %d\n", labeled);
    printf("Skipped: %d\n", skipped);
    printf("Labels saved to: %s\n", output_file);
    
    return 0;
}
