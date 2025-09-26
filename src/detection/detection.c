#include "detection.h"
#ifdef SDL2_IMAGE_AVAILABLE
#include <SDL2/SDL_image.h>
#endif

#include <string.h>

// SDL2 initialization and cleanup
int init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }
    
    return 1;
}

void cleanup_sdl() {
    SDL_Quit();
}

// SDL2 utility functions
Uint32 get_pixel(SDL_Surface *surface, int x, int y) {
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        return 0;
    }
    
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    
    switch (bpp) {
        case 1:
            return *p;
        case 2:
            return *(Uint16 *)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
        case 4:
            return *(Uint32 *)p;
        default:
            return 0;
    }
}

void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        return;
    }
    
    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    
    switch (bpp) {
        case 1:
            *p = pixel;
            break;
        case 2:
            *(Uint16 *)p = pixel;
            break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;
        case 4:
            *(Uint32 *)p = pixel;
            break;
    }
}

SDL_Surface* create_surface(int width, int height, Uint32 format) {
    (void)format; // Suppress unused parameter warning
    SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    if (!surface) {
        printf("Unable to create surface! SDL Error: %s\n", SDL_GetError());
        return NULL;
    }
    return surface;
}

// Image I/O functions
Image* load_image(const char* filename) {
    SDL_Surface *surface = NULL;
    const char *ext = strrchr(filename, '.');
    
    // Try to load based on file extension
    if (ext && (strcmp(ext, ".bmp") == 0 || strcmp(ext, ".BMP") == 0)) {
        // Load BMP file using SDL2's built-in BMP support
        surface = SDL_LoadBMP(filename);
        if (!surface) {
            printf("Failed to load BMP image '%s': %s\n", filename, SDL_GetError());
        }
    } else if (ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0)) {
#ifdef SDL2_IMAGE_AVAILABLE
        // Load PNG file using SDL2_image
        surface = IMG_Load(filename);
        if (!surface) {
            printf("Failed to load PNG image '%s': %s\n", filename, IMG_GetError());
        }
#else
        // PNG support not available
        printf("PNG support requires SDL2_image library. Creating test image instead...\n");
        printf("To enable PNG support, install SDL2_image: sudo apt-get install libsdl2-image-dev\n");
        printf("Then compile with: make CFLAGS+=-DSDL2_IMAGE_AVAILABLE\n");
        return create_image(400, 400, 3);
#endif
    } else {
        // Try BMP first, then fallback
        surface = SDL_LoadBMP(filename);
        if (!surface) {
            printf("Failed to load image '%s': %s\n", filename, SDL_GetError());
            printf("Supported formats: BMP");
#ifdef SDL2_IMAGE_AVAILABLE
            printf(", PNG");
#endif
            printf("\n");
            printf("For PNG support, install SDL2_image: sudo apt-get install libsdl2-image-dev\n");
        }
    }
    
    if (!surface) {
        printf("Creating test image instead...\n");
        return create_image(400, 400, 3);
    }
    
    // Convert to RGBA format for consistent processing
    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    
    if (!converted) {
        printf("Failed to convert image: %s\n", SDL_GetError());
        return create_image(400, 400, 3);
    }
    
    Image* img = malloc(sizeof(Image));
    if (!img) {
        SDL_FreeSurface(converted);
        return NULL;
    }
    
    img->surface = converted;
    img->width = converted->w;
    img->height = converted->h;
    img->channels = 4; // RGBA
    
    printf("✓ Image loaded successfully: %dx%d pixels\n", img->width, img->height);
    return img;
}

Image* create_image(int width, int height, int channels) {
    Image* img = malloc(sizeof(Image));
    if (!img) return NULL;
    
    Uint32 format = (channels == 1) ? SDL_PIXELFORMAT_RGBA32 : SDL_PIXELFORMAT_RGBA32;
    img->surface = create_surface(width, height, format);
    
    if (!img->surface) {
        free(img);
        return NULL;
    }
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    
    return img;
}

void free_image(Image* img) {
    if (img) {
        if (img->surface) {
            SDL_FreeSurface(img->surface);
        }
        free(img);
    }
}

int save_image(const char* filename, Image* img) {
    if (!img || !img->surface) {
        return 0;
    }
    
    const char *ext = strrchr(filename, '.');
    char output_filename[256];
    strncpy(output_filename, filename, sizeof(output_filename) - 1);
    output_filename[sizeof(output_filename) - 1] = '\0';
    
    // Determine output format based on extension
    if (ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".PNG") == 0)) {
#ifdef SDL2_IMAGE_AVAILABLE
        // Save as PNG using SDL2_image
        int result = IMG_SavePNG(img->surface, output_filename);
        if (result != 0) {
            printf("Failed to save PNG image '%s': %s\n", output_filename, IMG_GetError());
            return 0;
        }
        printf("✓ Image saved as PNG: %s\n", output_filename);
        return 1;
#else
        // PNG saving not available
        printf("PNG saving requires SDL2_image library. Saving as BMP instead...\n");
        printf("To enable PNG support, install SDL2_image: sudo apt-get install libsdl2-image-dev\n");
        printf("Then compile with: make CFLAGS+=-DSDL2_IMAGE_AVAILABLE\n");
        
        // Change extension to .bmp
        char *bmp_ext = strrchr(output_filename, '.');
        if (bmp_ext) {
            strcpy(bmp_ext, ".bmp");
        } else {
            strcat(output_filename, ".bmp");
        }
        
        // Save as BMP
        int result = SDL_SaveBMP(img->surface, output_filename);
        if (result != 0) {
            printf("Failed to save BMP image '%s': %s\n", output_filename, SDL_GetError());
            return 0;
        }
        printf("✓ Image saved as BMP: %s\n", output_filename);
        return 1;
#endif
    } else {
        // Ensure .bmp extension for other cases
        char *bmp_ext = strrchr(output_filename, '.');
        if (bmp_ext) {
            strcpy(bmp_ext, ".bmp");
        } else {
            strcat(output_filename, ".bmp");
        }
        
        // Save as BMP using SDL2's built-in BMP support
        int result = SDL_SaveBMP(img->surface, output_filename);
        if (result != 0) {
            printf("Failed to save image '%s': %s\n", output_filename, SDL_GetError());
            return 0;
        }
        
        printf("✓ Image saved as BMP: %s\n", output_filename);
        return 1;
    }
}

// Utility functions
double calculate_distance(Point p1, Point p2) {
    int dx = p1.x - p2.x;
    int dy = p1.y - p2.y;
    return sqrt(dx * dx + dy * dy);
}

Point calculate_center(Point p1, Point p2) {
    Point center;
    center.x = (p1.x + p2.x) / 2;
    center.y = (p1.y + p2.y) / 2;
    return center;
}

// Image processing functions
Image* convert_to_grayscale(Image* img) {
    if (!img || !img->surface) return NULL;
    
    Image* gray = create_image(img->width, img->height, 1);
    if (!gray) return NULL;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            Uint32 pixel = get_pixel(img->surface, x, y);
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, img->surface->format, &r, &g, &b, &a);
            
            // Convert to grayscale using standard weights
            Uint8 gray_value = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
            Uint32 gray_pixel = SDL_MapRGBA(gray->surface->format, gray_value, gray_value, gray_value, 255);
            set_pixel(gray->surface, x, y, gray_pixel);
        }
    }
    
    return gray;
}

Image* apply_gaussian_blur(Image* img, int kernel_size) {
    if (!img || !img->surface) return NULL;
    
    Image* blurred = create_image(img->width, img->height, img->channels);
    if (!blurred) return NULL;
    
    int half_kernel = kernel_size / 2;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int r_sum = 0, g_sum = 0, b_sum = 0, a_sum = 0;
            int count = 0;
            
            for (int ky = -half_kernel; ky <= half_kernel; ky++) {
                for (int kx = -half_kernel; kx <= half_kernel; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    if (nx >= 0 && nx < img->width && ny >= 0 && ny < img->height) {
                        Uint32 pixel = get_pixel(img->surface, nx, ny);
                        Uint8 r, g, b, a;
                        SDL_GetRGBA(pixel, img->surface->format, &r, &g, &b, &a);
                        
                        r_sum += r;
                        g_sum += g;
                        b_sum += b;
                        a_sum += a;
                        count++;
                    }
                }
            }
            
            Uint8 r = r_sum / count;
            Uint8 g = g_sum / count;
            Uint8 b = b_sum / count;
            Uint8 a = a_sum / count;
            
            Uint32 blurred_pixel = SDL_MapRGBA(blurred->surface->format, r, g, b, a);
            set_pixel(blurred->surface, x, y, blurred_pixel);
        }
    }
    
    return blurred;
}

Image* apply_edge_detection(Image* img) {
    if (!img || !img->surface) return NULL;
    
    Image* edges = create_image(img->width, img->height, 1);
    if (!edges) return NULL;
    
    // Sobel edge detection
    int sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (int y = 1; y < img->height - 1; y++) {
        for (int x = 1; x < img->width - 1; x++) {
            int gx = 0, gy = 0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    Uint32 pixel = get_pixel(img->surface, x + kx, y + ky);
                    Uint8 r, g, b, a;
                    SDL_GetRGBA(pixel, img->surface->format, &r, &g, &b, &a);
                    
                    int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
                    gx += gray * sobel_x[ky + 1][kx + 1];
                    gy += gray * sobel_y[ky + 1][kx + 1];
                }
            }
            
            int magnitude = (int)sqrt(gx * gx + gy * gy);
            if (magnitude > 255) magnitude = 255;
            
            Uint32 edge_pixel = SDL_MapRGBA(edges->surface->format, magnitude, magnitude, magnitude, 255);
            set_pixel(edges->surface, x, y, edge_pixel);
        }
    }
    
    return edges;
}

Image* apply_threshold(Image* img, int threshold) {
    if (!img || !img->surface) return NULL;
    
    Image* thresh = create_image(img->width, img->height, 1);
    if (!thresh) return NULL;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            Uint32 pixel = get_pixel(img->surface, x, y);
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, img->surface->format, &r, &g, &b, &a);
            
            int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            Uint8 value = (gray > threshold) ? 255 : 0;
            
            Uint32 thresh_pixel = SDL_MapRGBA(thresh->surface->format, value, value, value, 255);
            set_pixel(thresh->surface, x, y, thresh_pixel);
        }
    }
    
    return thresh;
}

// Grid detection functions
Rectangle detect_grid_boundaries(Image* img) {
    Rectangle rect = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, 0, 0};
    
    if (!img || !img->surface) return rect;
    
    // Convert to grayscale and apply preprocessing
    Image* gray = convert_to_grayscale(img);
    if (!gray) return rect;
    
    Image* blurred = apply_gaussian_blur(gray, 3);
    if (!blurred) {
        free_image(gray);
        return rect;
    }
    
    Image* edges = apply_edge_detection(blurred);
    if (!edges) {
        free_image(gray);
        free_image(blurred);
        return rect;
    }
    
    Image* thresh = apply_threshold(edges, 50);
    if (!thresh) {
        free_image(gray);
        free_image(blurred);
        free_image(edges);
        return rect;
    }
    
    // Find the largest rectangular region
    int min_x = img->width, max_x = 0, min_y = img->height, max_y = 0;
    int found_edges = 0;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            Uint32 pixel = get_pixel(thresh->surface, x, y);
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, thresh->surface->format, &r, &g, &b, &a);
            
            if (r > 0) { // Edge pixel found
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                found_edges = 1;
            }
        }
    }
    
    if (found_edges) {
        rect.top_left = (Point){min_x, min_y};
        rect.top_right = (Point){max_x, min_y};
        rect.bottom_left = (Point){min_x, max_y};
        rect.bottom_right = (Point){max_x, max_y};
        rect.width = max_x - min_x;
        rect.height = max_y - min_y;
    }
    
    // Clean up
    free_image(gray);
    free_image(blurred);
    free_image(edges);
    free_image(thresh);
    
    return rect;
}

int detect_grid_dimensions(Image* img, Rectangle bounds) {
    if (!img || !img->surface) return 0;
    
    // Count vertical and horizontal lines within the grid bounds
    int vertical_lines = 0, horizontal_lines = 0;
    
    // Count vertical lines
    for (int x = bounds.top_left.x; x < bounds.top_right.x; x += 10) {
        int line_pixels = 0;
        for (int y = bounds.top_left.y; y < bounds.bottom_left.y; y++) {
            Uint32 pixel = get_pixel(img->surface, x, y);
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, img->surface->format, &r, &g, &b, &a);
            
            int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            if (gray < 128) { // Dark pixel
                line_pixels++;
            }
        }
        if (line_pixels > bounds.height * 0.3) { // Threshold for line detection
            vertical_lines++;
        }
    }
    
    // Count horizontal lines
    for (int y = bounds.top_left.y; y < bounds.bottom_left.y; y += 10) {
        int line_pixels = 0;
        for (int x = bounds.top_left.x; x < bounds.top_right.x; x++) {
            Uint32 pixel = get_pixel(img->surface, x, y);
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, img->surface->format, &r, &g, &b, &a);
            
            int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            if (gray < 128) { // Dark pixel
                line_pixels++;
            }
        }
        if (line_pixels > bounds.width * 0.3) { // Threshold for line detection
            horizontal_lines++;
        }
    }
    
    // Return estimated grid size (simplified)
    return (vertical_lines + horizontal_lines) / 2;
}

char detect_letter_in_cell(Image* img, Point center, int cell_width, int cell_height) {
    // This is a simplified letter detection
    // In practice, you'd use OCR libraries like Tesseract
    
    // Suppress unused parameter warnings
    (void)img;
    (void)center;
    (void)cell_width;
    (void)cell_height;
    
    // For now, return a placeholder
    // You would implement actual OCR here
    return '?';
}

Grid* extract_grid(Image* img, Rectangle bounds) {
    if (!img || !img->surface) return NULL;
    
    Grid* grid = malloc(sizeof(Grid));
    if (!grid) return NULL;
    
    grid->bounds = bounds;
    
    // Estimate grid dimensions
    int estimated_size = detect_grid_dimensions(img, bounds);
    if (estimated_size <= 0) estimated_size = 10; // Default fallback
    
    grid->rows = estimated_size;
    grid->cols = estimated_size;
    
    // Allocate memory for letters and cell centers
    grid->letters = malloc(grid->rows * sizeof(char*));
    grid->cell_centers = malloc(grid->rows * sizeof(Point*));
    
    if (!grid->letters || !grid->cell_centers) {
        free_grid(grid);
        return NULL;
    }
    
    for (int i = 0; i < grid->rows; i++) {
        grid->letters[i] = malloc(grid->cols * sizeof(char));
        grid->cell_centers[i] = malloc(grid->cols * sizeof(Point));
        
        if (!grid->letters[i] || !grid->cell_centers[i]) {
            free_grid(grid);
            return NULL;
        }
    }
    
    // Calculate cell dimensions
    int cell_width = bounds.width / grid->cols;
    int cell_height = bounds.height / grid->rows;
    
    // Extract letters from each cell
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            Point cell_center = {
                bounds.top_left.x + col * cell_width + cell_width / 2,
                bounds.top_left.y + row * cell_height + cell_height / 2
            };
            
            grid->cell_centers[row][col] = cell_center;
            grid->letters[row][col] = detect_letter_in_cell(img, cell_center, cell_width, cell_height);
        }
    }
    
    return grid;
}

// Main detection function
Grid* detect_wordsearch_grid(Image* img) {
    printf("Starting grid detection...\n");
    
    if (!img || !img->surface) {
        printf("Invalid image provided.\n");
        return NULL;
    }
    
    // Step 1: Detect grid boundaries
    printf("Detecting grid boundaries...\n");
    Rectangle bounds = detect_grid_boundaries(img);
    
    if (bounds.width == 0 || bounds.height == 0) {
        printf("No grid detected in the image.\n");
        return NULL;
    }
    
    printf("Grid detected at: (%d, %d) to (%d, %d)\n", 
           bounds.top_left.x, bounds.top_left.y, 
           bounds.bottom_right.x, bounds.bottom_right.y);
    
    // Step 2: Extract the grid
    printf("Extracting grid...\n");
    Grid* grid = extract_grid(img, bounds);
    
    printf("Grid extraction completed.\n");
    return grid;
}

// Memory management functions
void free_grid(Grid* grid) {
    if (grid) {
        if (grid->letters) {
            for (int i = 0; i < grid->rows; i++) {
                free(grid->letters[i]);
            }
            free(grid->letters);
        }
        if (grid->cell_centers) {
            for (int i = 0; i < grid->rows; i++) {
                free(grid->cell_centers[i]);
            }
            free(grid->cell_centers);
        }
        free(grid);
    }
}