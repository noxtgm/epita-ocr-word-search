#include "label_cells.h"

// Display image as ASCII art for visual inspection during manual labeling
void show_ascii_preview(unsigned char *img, int w, int h) {
    printf("\nPreview:\n");
    int step_x = w / 40;
    int step_y = h / 20;
    if (step_x < 1) step_x = 1;
    if (step_y < 1) step_y = 1;
    
    // Convert grayscale values to ASCII characters
    for (int y = 0; y < h; y += step_y) {
        for (int x = 0; x < w; x += step_x) {
            int idx = y * w + x;
            unsigned char val = img[idx];
            
            // Map brightness to different ASCII characters
            if (val < 64) printf("██");       // Very dark
            else if (val < 128) printf("▓▓");  // Dark
            else if (val < 192) printf("▒▒");  // Light
            else printf("  ");                 // Very light
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_dir> <output_file>\n", argv[0]);
        printf("Example: %s ../../outputs/detection_grid/cells labels.txt\n", argv[0]);
        return 1;
    }
    
    const char *input_dir = argv[1];
    const char *output_file = argv[2];
    
    // Open output file in append mode to avoid overwriting existing labels
    FILE *out = fopen(output_file, "a");
    if (!out) {
        fprintf(stderr, "Cannot open output file: %s\n", output_file);
        return 1;
    }
    
    DIR *dir = opendir(input_dir);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", input_dir);
        fclose(out);
        return 1;
    }
    
    // Count total PNG files for progress tracking
    struct dirent *entry;
    int total_files = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char *ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".png") == 0) total_files++;
    }
    rewinddir(dir);
    
    printf("=== Simple Cell Labeling Tool ===\n");
    printf("Found %d PNG files in %s\n", total_files, input_dir);
    printf("\nInstructions:\n");
    printf("  - Type letter A-Z for the cell\n");
    printf("  - Type '7' to skip\n");
    printf("  - Type '8' to quit\n\n");
    
    int processed = 0, labeled = 0, skipped = 0;
    
    // Process each PNG file
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".png") != 0) continue;
        
        char img_path[1024];
        snprintf(img_path, sizeof(img_path), "%s/%s", input_dir, entry->d_name);
        
        // Load image in grayscale
        int width, height, channels;
        unsigned char *img = stbi_load(img_path, &width, &height, &channels, 1);
        
        if (!img) {
            fprintf(stderr, "Failed to load: %s\n", img_path);
            continue;
        }
        
        processed++;
        
        // Display image information and ASCII preview
        printf("\n[%d/%d] ================================\n", processed, total_files);
        printf("File: %s\n", entry->d_name);
        printf("Size: %dx%d\n", width, height);
        show_ascii_preview(img, width, height);
        stbi_image_free(img);
        
        // Get user input
        printf("\nLabel (A-Z/s/q): ");
        fflush(stdout);
        
        char input[10];
        if (!fgets(input, sizeof(input), stdin)) break;
        
        input[strcspn(input, "\n")] = 0;  // Remove newline
        
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
            fprintf(out, "%c %s/%s\n", label, input_dir, entry->d_name);
            fflush(out);
            labeled++;
            printf("✓ Labeled as '%c'\n", label);
        } else {
            printf("Invalid input, skipping.\n");
        }
    }
    
    closedir(dir);
    fclose(out);
    
    printf("\n=== Summary ===\n");
    printf("Total processed: %d\n", processed);
    printf("Labeled: %d\n", labeled);
    printf("Skipped: %d\n", skipped);
    printf("Labels saved to: %s\n", output_file);
    
    return 0;
}
