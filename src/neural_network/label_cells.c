#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Simple terminal-based labeling tool
void display_image_info(const char *filename, int width, int height) {
    printf("\n================================\n");
    printf("Image: %s\n", filename);
    printf("Size: %dx%d pixels\n", width, height);
    printf("================================\n");
}

void create_letter_directories(const char *base_path) {
    char dir_path[512];
    for (char c = 'A'; c <= 'Z'; c++) {
        snprintf(dir_path, sizeof(dir_path), "%s/%c", base_path, c);
        mkdir(dir_path, 0755);
    }
    printf("Created directories A-Z in %s\n", base_path);
}

void copy_file(const char *src, const char *dst) {
    FILE *source = fopen(src, "rb");
    if (!source) {
        perror("Failed to open source");
        return;
    }
    
    FILE *dest = fopen(dst, "wb");
    if (!dest) {
        perror("Failed to open destination");
        fclose(source);
        return;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, dest);
    }
    
    fclose(source);
    fclose(dest);
}

void show_ascii_preview(unsigned char *img, int w, int h) {
    // Show a simple ASCII preview
    printf("\nPreview (scaled down):\n");
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

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input_dir> <output_dir>\n", argv[0]);
        printf("Example: %s outputs/detection_grid/cells outputs/labeled_cells\n", argv[0]);
        return 1;
    }
    
    const char *input_dir = argv[1];
    const char *output_dir = argv[2];
    
    // Create output directory and subdirectories
    mkdir(output_dir, 0755);
    create_letter_directories(output_dir);
    
    // Create skip directory for non-letters
    char skip_dir[512];
    snprintf(skip_dir, sizeof(skip_dir), "%s/SKIP", output_dir);
    mkdir(skip_dir, 0755);
    
    // Open input directory
    DIR *dir = opendir(input_dir);
    if (!dir) {
        fprintf(stderr, "Cannot open directory: %s\n", input_dir);
        return 1;
    }
    
    struct dirent *entry;
    int total = 0, labeled = 0;
    
    printf("=== Cell Image Labeling Tool ===\n");
    printf("Instructions:\n");
    printf("  - Type the letter (A-Z) that you see\n");
    printf("  - Press 's' to skip (non-letter/unclear)\n");
    printf("  - Press 'q' to quit and save progress\n");
    printf("  - Press 'u' to undo last labeling\n\n");
    
    char last_src[1024] = "";
    char last_dst[1024] = "";
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        // Check if it's a PNG file
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".png") != 0) continue;
        
        char img_path[1024];
        snprintf(img_path, sizeof(img_path), "%s/%s", input_dir, entry->d_name);
        
        // Load image
        int width, height, channels;
        unsigned char *img = stbi_load(img_path, &width, &height, &channels, 1);
        
        if (!img) {
            fprintf(stderr, "Failed to load: %s\n", img_path);
            continue;
        }
        
        total++;
        
        // Display image info and preview
        display_image_info(entry->d_name, width, height);
        show_ascii_preview(img, width, height);
        stbi_image_free(img);
        
        // Get user input
        printf("\nWhat letter is this? (A-Z/s=skip/q=quit/u=undo): ");
        fflush(stdout);
        
        char input[10];
        if (!fgets(input, sizeof(input), stdin)) break;
        
        char label = toupper(input[0]);
        
        if (label == 'Q') {
            printf("Quitting...\n");
            break;
        }
        
        if (label == 'U' && strlen(last_dst) > 0) {
            // Undo last operation
            remove(last_dst);
            printf("Undone: removed %s\n", last_dst);
            last_dst[0] = '\0';
            labeled--;
            continue;
        }
        
        char dest_path[1024];
        
        if (label == 'S') {
            // Skip - move to SKIP folder
            snprintf(dest_path, sizeof(dest_path), "%s/%s", skip_dir, entry->d_name);
            copy_file(img_path, dest_path);
            printf("Skipped: %s\n", entry->d_name);
        } else if (label >= 'A' && label <= 'Z') {
            // Copy to appropriate letter folder
            snprintf(dest_path, sizeof(dest_path), "%s/%c/%s", output_dir, label, entry->d_name);
            copy_file(img_path, dest_path);
            printf("Labeled as '%c': %s\n", label, entry->d_name);
            labeled++;
            
            // Save for undo
            strncpy(last_src, img_path, sizeof(last_src));
            strncpy(last_dst, dest_path, sizeof(last_dst));
        } else {
            printf("Invalid input. Skipping this image.\n");
            total--;
        }
    }
    
    closedir(dir);
    
    printf("\n=== Summary ===\n");
    printf("Total images processed: %d\n", total);
    printf("Successfully labeled: %d\n", labeled);
    printf("Output directory: %s\n", output_dir);
    
    return 0;
}
