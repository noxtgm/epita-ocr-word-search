#include "rotation.h"

// Performs bilinear interpolation for smooth pixel sampling
static guchar bilinear_interpolate(const guchar *pixels, int width, int height,
                                   int n_channels, double x, double y, int channel)
{
    if (pixels == NULL || width <= 0 || height <= 0 || n_channels <= 0 || channel < 0) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 0;
    }
    
    if (x < 0 || x >= width - 1 || y < 0 || y >= height - 1) {
        fprintf(stderr, "Error: Coordinates are out of bounds\n");
        return 0;
    }
    
    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    double dx = x - x0;
    double dy = y - y0;
    
    int rowstride = width * n_channels;
    
    // Get the four surrounding pixels
    guchar p00 = pixels[y0 * rowstride + x0 * n_channels + channel];
    guchar p10 = pixels[y0 * rowstride + x1 * n_channels + channel];
    guchar p01 = pixels[y1 * rowstride + x0 * n_channels + channel];
    guchar p11 = pixels[y1 * rowstride + x1 * n_channels + channel];
    
    // Bilinear interpolation
    double value = p00 * (1 - dx) * (1 - dy) +
                   p10 * dx * (1 - dy) +
                   p01 * (1 - dx) * dy +
                   p11 * dx * dy;
    
    return (guchar)(value + 0.5);
}

// Rotate an image by a given angle in degrees
GdkPixbuf* rotate_image(GdkPixbuf *pixbuf, double angle)
{
    if (pixbuf == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return NULL;
    }
    
    // Get source image properties
    int src_width = gdk_pixbuf_get_width(pixbuf);
    int src_height = gdk_pixbuf_get_height(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    gboolean has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
    
    const guchar *src_pixels = gdk_pixbuf_get_pixels(pixbuf);
    
    // Convert angle to radians
    double angle_rad = angle * PI / 180.0;
    double cos_a = cos(angle_rad);
    double sin_a = sin(angle_rad);
    
    // Calculate the dimensions of the rotated image
    // And find the bounding box of the rotated image
    double corners_x[4], corners_y[4];
    corners_x[0] = 0;
    corners_y[0] = 0;
    corners_x[1] = src_width * cos_a;
    corners_y[1] = src_width * sin_a;
    corners_x[2] = -src_height * sin_a;
    corners_y[2] = src_height * cos_a;
    corners_x[3] = src_width * cos_a - src_height * sin_a;
    corners_y[3] = src_width * sin_a + src_height * cos_a;
    
    double min_x = corners_x[0], max_x = corners_x[0];
    double min_y = corners_y[0], max_y = corners_y[0];
    
    for (int i = 1; i < 4; i++) {
        if (corners_x[i] < min_x) {
            min_x = corners_x[i];
        }
        if (corners_x[i] > max_x) {
            max_x = corners_x[i];
        }
        if (corners_y[i] < min_y) {
            min_y = corners_y[i];
        }
        if (corners_y[i] > max_y) {
            max_y = corners_y[i];
        }
    }
    
    int dst_width = (int)(max_x - min_x + 0.5);
    int dst_height = (int)(max_y - min_y + 0.5);
    
    // Create the destination pixbuf
    GdkPixbuf *dst_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha,
                                           8, dst_width, dst_height);
    if (dst_pixbuf == NULL) {
        fprintf(stderr, "Error: Failed to create destination pixbuf\n");
        return NULL;
    }
    
    guchar *dst_pixels = gdk_pixbuf_get_pixels(dst_pixbuf);
    int dst_rowstride = gdk_pixbuf_get_rowstride(dst_pixbuf);
    
    // Center of source image
    double src_cx = src_width / 2.0;
    double src_cy = src_height / 2.0;
    
    // Center of destination image
    double dst_cx = dst_width / 2.0;
    double dst_cy = dst_height / 2.0;
    
    // Fill destination image
    for (int dst_y = 0; dst_y < dst_height; dst_y++) {
        for (int dst_x = 0; dst_x < dst_width; dst_x++) {
            // Translate to origin (center of dst image)
            double x = dst_x - dst_cx;
            double y = dst_y - dst_cy;
            
            // Apply inverse rotation to find source coordinates
            double src_x = x * cos_a + y * sin_a + src_cx;
            double src_y = -x * sin_a + y * cos_a + src_cy;
            
            // Check if source coordinates are within bounds
            if (src_x >= 0 && src_x < src_width - 1 &&
                src_y >= 0 && src_y < src_height - 1) {
                // Use bilinear interpolation for each channel
                for (int c = 0; c < n_channels; c++) {
                    dst_pixels[dst_y * dst_rowstride + dst_x * n_channels + c] =
                        bilinear_interpolate(src_pixels, src_width, src_height,
                                           n_channels, src_x, src_y, c);
                }
            } else {
                // Outside source image bounds - fill with white/transparent
                for (int c = 0; c < n_channels; c++) {
                    if (c == n_channels - 1 && has_alpha) {
                        dst_pixels[dst_y * dst_rowstride + dst_x * n_channels + c] = 0;
                    } else {
                        dst_pixels[dst_y * dst_rowstride + dst_x * n_channels + c] = 255;
                    }
                }
            }
        }
    }
    
    return dst_pixbuf;
}

// Load an image from a file
GdkPixbuf* load_image(const char *filename)
{
    if (filename == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return NULL;
    }

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    
    if (pixbuf == NULL) {
        if (error != NULL) {
            fprintf(stderr, "Error: Failed to load image '%s': %s\n", 
                    filename, error->message);
            g_error_free(error);
        } else {
            fprintf(stderr, "Error: Failed to load image '%s'\n", filename);
        }
        return NULL;
    }
    
    return pixbuf;
}

// Save an image to a file
int save_image(GdkPixbuf *pixbuf, const char *filename)
{
    if (pixbuf == NULL || filename == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 0;
    }
    
    // Determine the file format from the extension
    const char *ext = strrchr(filename, '.');
    const char *format = "png"; // Default format
    
    if (ext != NULL) {
        ext++; // Skip the dot
        if (g_ascii_strcasecmp(ext, "jpg") == 0 || g_ascii_strcasecmp(ext, "jpeg") == 0) {
            format = "jpeg";
        } else if (g_ascii_strcasecmp(ext, "png") == 0) {
            format = "png";
        } else if (g_ascii_strcasecmp(ext, "bmp") == 0) {
            format = "bmp";
        }
    }
    
    GError *error = NULL;
    gboolean result = gdk_pixbuf_save(pixbuf, filename, format, &error, NULL);
    
    if (!result) {
        if (error != NULL) {
            fprintf(stderr, "Error: Failed to save image '%s': %s\n", 
                    filename, error->message);
            g_error_free(error);
        } else {
            fprintf(stderr, "Error: Failed to save image '%s'\n", filename);
        }
        return 0;
    }
    
    return 1;
}

// Main rotation function which tests for edge cases and runs the rotation
// Returns: 0 = success, 1 = error
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_image> <angle_degrees>\n", argv[0]);
        return 1;
    }
    
    const char *file_name = argv[1];
    double angle = atof(argv[2]);
    
    // Initialize GdkPixbuf type system
    #ifndef GLIB_VERSION_2_36
        g_type_init();
    #endif
    
    // Load the image
    GdkPixbuf *input_pixbuf = load_image(file_name);
    if (input_pixbuf == NULL) {
        fprintf(stderr, "Error: Failed to load the image\n");
        return 1;
    }

    // Rotate the image
    GdkPixbuf *rotated_pixbuf = rotate_image(input_pixbuf, angle); 
    if (rotated_pixbuf == NULL) {
        g_object_unref(input_pixbuf);
        fprintf(stderr, "Error: Failed to rotate the image\n");
        return 1;
    }

    // Generate output filename
    const char *last_slash = strrchr(file_name, '/');
    const char *basename = last_slash ? last_slash + 1 : file_name;
    
    char out_file_name[512];
    snprintf(out_file_name, sizeof(out_file_name), "../../outputs/rotated/rotated_%s", basename);
    
    // Save the rotated image
    if (!save_image(rotated_pixbuf, out_file_name)) {
        g_object_unref(input_pixbuf);
        g_object_unref(rotated_pixbuf);
        return 1;
    }
    
    fprintf(stderr, "Image rotated and saved as: %s\n", out_file_name);
    
    g_object_unref(input_pixbuf);
    g_object_unref(rotated_pixbuf);
    return 0;
}
