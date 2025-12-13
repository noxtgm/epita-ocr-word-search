#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define PI 3.14159265358979323846
#define ANGLE_RANGE 45.0
#define ANGLE_STEP 0.5

typedef struct {
    int width;
    int height;
    guchar *pixels;
    int rowstride;
    int n_channels;
} ImageData;

// Calculate edge magnitude using Sobel operator
void detect_edges(ImageData *img, double **edge_magnitude, double **edge_angle) {
    int sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    *edge_magnitude = g_malloc(img->width * img->height * sizeof(double));
    *edge_angle = g_malloc(img->width * img->height * sizeof(double));
    
    for (int y = 1; y < img->height - 1; y++) {
        for (int x = 1; x < img->width - 1; x++) {
            double gx = 0, gy = 0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int px = x + kx;
                    int py = y + ky;
                    guchar *pixel = img->pixels + py * img->rowstride + px * img->n_channels;
                    double intensity = (pixel[0] + pixel[1] + pixel[2]) / 3.0;
                    
                    gx += intensity * sobel_x[ky + 1][kx + 1];
                    gy += intensity * sobel_y[ky + 1][kx + 1];
                }
            }
            
            int idx = y * img->width + x;
            (*edge_magnitude)[idx] = sqrt(gx * gx + gy * gy);
            (*edge_angle)[idx] = atan2(gy, gx);
        }
    }
}

// Project pixels to detect text lines
double detect_rotation_angle(ImageData *img) {
    int num_angles = (int)(2 * ANGLE_RANGE / ANGLE_STEP) + 1;
    double *angle_scores = g_malloc0(num_angles * sizeof(double));
    
    int cx = img->width / 2;
    int cy = img->height / 2;
    
    // Try different angles and measure variance in projections
    for (int a = 0; a < num_angles; a++) {
        double test_angle = (-ANGLE_RANGE + a * ANGLE_STEP) * PI / 180.0;
        double cos_a = cos(test_angle);
        double sin_a = sin(test_angle);
        
        // Project onto axis perpendicular to test angle
        int max_proj = (int)(img->width * fabs(sin_a) + img->height * fabs(cos_a)) + 1;
        double *projection = g_malloc0(max_proj * sizeof(double));
        int *counts = g_malloc0(max_proj * sizeof(int));
        
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                guchar *pixel = img->pixels + y * img->rowstride + x * img->n_channels;
                double intensity = (pixel[0] + pixel[1] + pixel[2]) / 3.0;
                
                // Check if pixel is dark (likely text)
                if (intensity < 200) {
                    int rx = x - cx;
                    int ry = y - cy;
                    int proj_idx = (int)((rx * sin_a - ry * cos_a) + max_proj / 2);
                    
                    if (proj_idx >= 0 && proj_idx < max_proj) {
                        projection[proj_idx] += (255 - intensity);
                        counts[proj_idx]++;
                    }
                }
            }
        }
        
        // Calculate variance of projection (higher = better alignment)
        double mean = 0;
        int total = 0;
        for (int i = 0; i < max_proj; i++) {
            mean += projection[i];
            total += counts[i];
        }
        if (total > 0) mean /= total;
        
        double variance = 0;
        for (int i = 0; i < max_proj; i++) {
            if (counts[i] > 0) {
                double diff = projection[i] / counts[i] - mean;
                variance += diff * diff * counts[i];
            }
        }
        
        angle_scores[a] = variance;
        
        g_free(projection);
        g_free(counts);
    }
    
    // Find angle with maximum variance (best alignment)
    int max_idx = 0;
    double max_score = angle_scores[0];
    
    for (int i = 1; i < num_angles; i++) {
        if (angle_scores[i] > max_score) {
            max_score = angle_scores[i];
            max_idx = i;
        }
    }
    
    double detected_angle = -ANGLE_RANGE + max_idx * ANGLE_STEP;
    
    // Fine-tune around the detected angle
    double fine_step = 0.1;
    double best_angle = detected_angle;
    max_score = angle_scores[max_idx];
    
    for (double fine_angle = detected_angle - ANGLE_STEP; 
         fine_angle <= detected_angle + ANGLE_STEP; 
         fine_angle += fine_step) {
        
        double test_angle = fine_angle * PI / 180.0;
        double cos_a = cos(test_angle);
        double sin_a = sin(test_angle);
        
        int max_proj = (int)(img->width * fabs(sin_a) + img->height * fabs(cos_a)) + 1;
        double *projection = g_malloc0(max_proj * sizeof(double));
        int *counts = g_malloc0(max_proj * sizeof(int));
        
        for (int y = 0; y < img->height; y += 2) {
            for (int x = 0; x < img->width; x += 2) {
                guchar *pixel = img->pixels + y * img->rowstride + x * img->n_channels;
                double intensity = (pixel[0] + pixel[1] + pixel[2]) / 3.0;
                
                if (intensity < 200) {
                    int rx = x - cx;
                    int ry = y - cy;
                    int proj_idx = (int)((rx * sin_a - ry * cos_a) + max_proj / 2);
                    
                    if (proj_idx >= 0 && proj_idx < max_proj) {
                        projection[proj_idx] += (255 - intensity);
                        counts[proj_idx]++;
                    }
                }
            }
        }
        
        double mean = 0;
        int total = 0;
        for (int i = 0; i < max_proj; i++) {
            mean += projection[i];
            total += counts[i];
        }
        if (total > 0) mean /= total;
        
        double variance = 0;
        for (int i = 0; i < max_proj; i++) {
            if (counts[i] > 0) {
                double diff = projection[i] / counts[i] - mean;
                variance += diff * diff * counts[i];
            }
        }
        
        if (variance > max_score) {
            max_score = variance;
            best_angle = fine_angle;
        }
        
        g_free(projection);
        g_free(counts);
    }
    
    g_free(angle_scores);
    
    return best_angle;
}

// Rotate image by given angle
GdkPixbuf* rotate_image(GdkPixbuf *original, double angle_deg) {
    int orig_width = gdk_pixbuf_get_width(original);
    int orig_height = gdk_pixbuf_get_height(original);
    
    double angle_rad = -angle_deg * PI / 180.0;
    double cos_a = cos(angle_rad);
    double sin_a = sin(angle_rad);
    
    // Calculate new dimensions
    int new_width = (int)(fabs(orig_width * cos_a) + fabs(orig_height * sin_a)) + 1;
    int new_height = (int)(fabs(orig_width * sin_a) + fabs(orig_height * cos_a)) + 1;
    
    GdkPixbuf *rotated = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 
                                        gdk_pixbuf_get_has_alpha(original),
                                        8, new_width, new_height);
    
    gdk_pixbuf_fill(rotated, 0xffffffff);
    
    guchar *orig_pixels = gdk_pixbuf_get_pixels(original);
    guchar *rot_pixels = gdk_pixbuf_get_pixels(rotated);
    int orig_rowstride = gdk_pixbuf_get_rowstride(original);
    int rot_rowstride = gdk_pixbuf_get_rowstride(rotated);
    int n_channels = gdk_pixbuf_get_n_channels(original);
    
    int cx_orig = orig_width / 2;
    int cy_orig = orig_height / 2;
    int cx_new = new_width / 2;
    int cy_new = new_height / 2;
    
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            int rel_x = x - cx_new;
            int rel_y = y - cy_new;
            
            int src_x = (int)(rel_x * cos_a + rel_y * sin_a + cx_orig);
            int src_y = (int)(-rel_x * sin_a + rel_y * cos_a + cy_orig);
            
            if (src_x >= 0 && src_x < orig_width && src_y >= 0 && src_y < orig_height) {
                guchar *src = orig_pixels + src_y * orig_rowstride + src_x * n_channels;
                guchar *dst = rot_pixels + y * rot_rowstride + x * n_channels;
                memcpy(dst, src, n_channels);
            }
        }
    }
    
    return rotated;
}

// GUI callbacks
typedef struct {
    GtkWidget *image_widget;
    GtkWidget *angle_label;
    GdkPixbuf *original_pixbuf;
} AppData;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        return 1;
    }
    
    gtk_init(&argc, &argv);
    
    const char *input_file = argv[1];
    GError *error = NULL;
    
    // Load the image
    GdkPixbuf *original = gdk_pixbuf_new_from_file(input_file, &error);
    if (!original) {
        fprintf(stderr, "Error loading image: %s\n", error->message);
        g_error_free(error);
        return 1;
    }
    
    // Detect rotation angle
    ImageData img_data = {
        .width = gdk_pixbuf_get_width(original),
        .height = gdk_pixbuf_get_height(original),
        .pixels = gdk_pixbuf_get_pixels(original),
        .rowstride = gdk_pixbuf_get_rowstride(original),
        .n_channels = gdk_pixbuf_get_n_channels(original)
    };
    
    printf("Detecting rotation angle...\n");
    double angle = detect_rotation_angle(&img_data);
    printf("Detected angle: %.2f degrees\n", angle);
    
    // Skip rotation if angle is negligible (less than 0.5 degrees)
    if (fabs(angle) < 0.5) {
        printf("No rotation required, skipping step\n");
        g_object_unref(original);
        return 0;
    }
    
    // Rotate the image
    GdkPixbuf *corrected = rotate_image(original, angle);
    
    // Generate output filename - same name as input, in outputs/rotation directory
    char output_file[1024];
    const char *basename = strrchr(input_file, '/');
    basename = basename ? basename + 1 : input_file;
    
    snprintf(output_file, sizeof(output_file), "../../outputs/rotation/%s", basename);
    
    // Save the corrected image
    gdk_pixbuf_save(corrected, output_file, "png", &error, NULL);
    if (error) {
        fprintf(stderr, "Error saving image: %s\n", error->message);
        g_error_free(error);
        g_object_unref(original);
        g_object_unref(corrected);
        return 1;
    }
    
    printf("Rotated image saved to: %s\n", output_file);
    
    g_object_unref(original);
    g_object_unref(corrected);
    
    return 0;
}