#include "detection.h"
#include <string.h>
#ifdef _WIN32
#include <direct.h>  // For _mkdir on Windows
#else
#include <sys/stat.h>  // For mkdir on Unix-like systems
#endif

// GTK initialization and cleanup
gboolean init_gtk(int *argc, char ***argv) {
    return gtk_init_check(argc, argv);
}

void cleanup_gtk() 
{

}

// Pixel cache management functions
void cache_pixel_data(Image* img) {
    if (!img || !img->pixbuf || img->cache_valid) return;
    gint size = img->width * img->height * img->channels;
    img->cached_pixels = g_malloc(size);
    if (!img->cached_pixels) {
        g_warning("Failed to allocate memory for pixel cache");
        return;
    }
    // Convert GdkPixbuf to linear array
    guchar *pixels = gdk_pixbuf_get_pixels(img->pixbuf);
    gint rowstride = gdk_pixbuf_get_rowstride(img->pixbuf);
    gint n_channels = gdk_pixbuf_get_n_channels(img->pixbuf);
    
    for (gint y = 0; y < img->height; y++) {
        for (gint x = 0; x < img->width; x++) {
            gint cache_idx = (y * img->width + x) * img->channels;
            gint pixbuf_idx = y * rowstride + x * n_channels;
            for (gint c = 0; c < img->channels; c++) 
            {
                img->cached_pixels[cache_idx + c] = pixels[pixbuf_idx + c];
            }
        }
    }
    img->cache_valid = TRUE;
}

void free_pixel_cache(Image* img) {
    if (!img) return;
    g_free(img->cached_pixels);
    img->cached_pixels = NULL;
    img->cache_valid = FALSE;
}

guchar get_cached_pixel(Image* img, gint x, gint y, gint channel) {
    if (!img || x < 0 || x >= img->width || y < 0 || y >= img->height || channel < 0 || channel >= img->channels) {
        return 0;
    }
    if (!img->cache_valid) {
        cache_pixel_data(img);
    }
    gint idx = (y * img->width + x) * img->channels + channel;
    return img->cached_pixels[idx];
}

void set_cached_pixel(Image* img, gint x, gint y, gint channel, guchar value) {
    if (!img || x < 0 || x >= img->width || y < 0 || y >= img->height || channel < 0 || channel >= img->channels) {
        return;
    }
    if (!img->cache_valid) {
        cache_pixel_data(img);
    }
    gint idx = (y * img->width + x) * img->channels + channel;
    img->cached_pixels[idx] = value;
}

void sync_cache_to_pixbuf(Image* img) {
    if (!img || !img->pixbuf || !img->cache_valid) return;
    
    guchar *pixels = gdk_pixbuf_get_pixels(img->pixbuf);
    gint rowstride = gdk_pixbuf_get_rowstride(img->pixbuf);
    gint n_channels = gdk_pixbuf_get_n_channels(img->pixbuf);
    
    for (gint y = 0; y < img->height; y++) {
        for (gint x = 0; x < img->width; x++) {
            gint cache_idx = (y * img->width + x) * img->channels;
            gint pixbuf_idx = y * rowstride + x * n_channels;
            
            pixels[pixbuf_idx + 0] = img->cached_pixels[cache_idx + 0]; // R
            pixels[pixbuf_idx + 1] = (img->channels > 1) ? img->cached_pixels[cache_idx + 1] : img->cached_pixels[cache_idx + 0]; // G
            pixels[pixbuf_idx + 2] = (img->channels > 2) ? img->cached_pixels[cache_idx + 2] : img->cached_pixels[cache_idx + 0]; // B
            if (n_channels == 4) {
                pixels[pixbuf_idx + 3] = (img->channels > 3) ? img->cached_pixels[cache_idx + 3] : 255; // A
            }
        }
    }
}

// Image I/O functions
Image* load_image(const char* filename) {
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    
    if (!pixbuf) {
        g_warning("Failed to load image '%s': %s", filename, error->message);
        g_error_free(error);
        return NULL;
    }
    
    // Ensure RGBA format (add alpha channel if missing)
    if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
        GdkPixbuf *rgba_pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
        g_object_unref(pixbuf);
        pixbuf = rgba_pixbuf;
    }
    
    Image* img = g_new0(Image, 1);
    if (!img) {
        g_object_unref(pixbuf);
        return NULL;
    }
    
    img->pixbuf = pixbuf;
    img->width = gdk_pixbuf_get_width(pixbuf);
    img->height = gdk_pixbuf_get_height(pixbuf);
    img->channels = gdk_pixbuf_get_n_channels(pixbuf);
    img->cached_pixels = NULL;
    img->cache_valid = FALSE;
    
    g_print("Image loaded successfully: %dx%d pixels\n", img->width, img->height);
    return img;
}

Image* create_image(int width, int height, int channels)
{   
    Image* img = g_new0(Image, 1);
    if (!img) return NULL;
    
    gboolean has_alpha = (channels == 4);
    img->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8, width, height);
    
    if (!img->pixbuf) {
        g_free(img);
        return NULL;
    }
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->cached_pixels = NULL;
    img->cache_valid = FALSE;
    
    return img;
}

void free_image(Image* img) 
{
    if (img) 
    {
        if (img->pixbuf) {
            g_object_unref(img->pixbuf);
        }
        free_pixel_cache(img);
        g_free(img);
    }
}

int save_image(const char* filename, Image* img) 
{
    if (!img || !img->pixbuf) {
        return 0;
    }
    if (img->cache_valid) {
        sync_cache_to_pixbuf(img);
    }
    // Save as PNG using GdkPixbuf
    GError *error = NULL;
    gboolean success = gdk_pixbuf_save(img->pixbuf, filename, "png", &error, NULL);
    
    if (!success) {
        g_warning("Failed to save image '%s': %s", filename, error->message);
        g_error_free(error);
        return 0;
    }
    g_print("Image saved as PNG: %s\n", filename);
    return 1;
}

// Memory management functions
void free_grid(Grid* grid) 
{
    if (grid) {
        if (grid->letters) {
            for (int i = 0; i < grid->rows; i++) {
                free(grid->letters[i]);
            }
            free(grid->letters);
        }
        if (grid->cells) {
            for (int r = 0; r < grid->rows; r++) {
                if (grid->cells[r]) {
                    for (int c = 0; c < grid->cols; c++) {
                        if (grid->cells[r][c].image) {
                            free_image(grid->cells[r][c].image);
                        }
                        if (grid->cells[r][c].image_file) {
                            g_free(grid->cells[r][c].image_file);
                        }
                    }
                    free(grid->cells[r]);
                }
            }
            free(grid->cells);
        }
        free(grid);
    }
}

Image* morph_erode(Image* img, int kernel_w, int kernel_h) {
    if (!img || img->channels < 1) return NULL;
    if (kernel_w < 1) kernel_w = 1;
    if (kernel_h < 1) kernel_h = 1;
    int half_w = kernel_w / 2;
    int half_h = kernel_h / 2;

    Image* out = create_image(img->width, img->height, 1);
    if (!out) return NULL;

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            guchar min_val = 255;
            for (int ky = -half_h; ky <= half_h; ky++) {
                for (int kx = -half_w; kx <= half_w; kx++) 
                {
                    int nx = CLAMP(x + kx, 0, img->width - 1);
                    int ny = CLAMP(y + ky, 0, img->height - 1);
                    guchar v = get_cached_pixel(img, nx, ny, 0);
                    if (v < min_val) min_val = v;
                }
            }
            set_cached_pixel(out, x, y, 0, min_val);
        }
    }
    return out;
}

Image* morph_dilate(Image* img, int kernel_w, int kernel_h) {
    if (!img || img->channels < 1) return NULL;
    if (kernel_w < 1) kernel_w = 1;
    if (kernel_h < 1) kernel_h = 1;
    int half_w = kernel_w / 2;
    int half_h = kernel_h / 2;

    Image* out = create_image(img->width, img->height, 1);
    if (!out) return NULL;

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            guchar max_val = 0;
            for (int ky = -half_h; ky <= half_h; ky++) {
                for (int kx = -half_w; kx <= half_w; kx++) {
                    int nx = CLAMP(x + kx, 0, img->width - 1);
                    int ny = CLAMP(y + ky, 0, img->height - 1);
                    guchar v = get_cached_pixel(img, nx, ny,0);
                    if (v > max_val) max_val = v;
                }
            }
            set_cached_pixel(out, x, y, 0, max_val);
        }
    }
    return out;
}

Image* morph_open(Image* img, int kernel_w, int kernel_h) {
    if (!img) return NULL;
    Image* er = morph_erode(img, kernel_w, kernel_h);
    if (!er) return NULL;
    Image* dl = morph_dilate(er, kernel_w, kernel_h);
    free_image(er);
    return dl;
}

Image* open_vertical_lines(Image* img, int kernel_height, int thickness_restore) {
    kernel_height = fmax(kernel_height,1);
    thickness_restore = fmax(thickness_restore,1);
    // Erode and dilate with a tall thin kernel
    Image* opened = morph_open(img, 3, kernel_height);
    if (!opened) return NULL;
    if (thickness_restore > 1) {
        Image* restored = morph_dilate(opened, thickness_restore, 1);
        free_image(opened);
        return restored;
    }
    return opened;
}

Image* open_horizontal_lines(Image* img, int kernel_width, int thickness_restore) {
    kernel_width = fmax(kernel_width,1);
    thickness_restore = fmax(thickness_restore,1);
    // Erode and dilate with a wide thin kernel
    Image* opened = morph_open(img, kernel_width, 3);
    if (!opened) return NULL;
    if (thickness_restore > 1) {
        Image* restored = morph_dilate(opened, 1, thickness_restore);
        free_image(opened);
        return restored;
    }
    return opened;
}

// Projections and peak detection
int* column_projection(Image* img) {
    if (!img) return NULL;
    int* sum = g_malloc0(sizeof(int) * img->width);
    if (!sum) return NULL;
    for (int x = 0; x < img->width; x++) {
        gint acc = 0;
        for (int y = 0; y < img->height; y++) {
            acc += (int)get_cached_pixel(img, x, y, 0);  
        }
        sum[x] = acc;
    }
    return sum;
}

int* row_projection(Image* img) {
    if (!img) return NULL;
    int* sum = g_malloc0(sizeof(int) * img->height);
    if (!sum) return NULL;
    for (int y = 0; y < img->height; y++) {
        gint acc = 0;
        for (int x = 0; x < img->width; x++) {
            acc += (int)get_cached_pixel(img, x, y, 0);
        }
        sum[y] = acc;
    }
    return sum;
}


int profile_auto_threshold(const int* values, int n) {
    if (!values || n <= 0) return 0;
    long long sum = 0;
    int mx = 0;
    for (int i = 0; i < n; i++) { sum += values[i]; if (values[i] > mx) mx = values[i]; }
    double mean = (double)sum / (double)n;
    int thr = (int)(mean + 0.5 * (mx - mean)); // halfway between mean and max
    return thr;
}

int find_peaks(const int* values, int n, int min_spacing, int threshold, int* out_indices, int max_out) {
    if (!values || !out_indices || n <= 0 || max_out <= 0) return 0;
    int count = 0;
    int nms = 2; // local neighborhood check
    for (int i = 0; i < n; i++) {
        int v = values[i];
        if (v < threshold) continue;
        // local maximum check
        int is_peak = 1;
        for (int d = -nms; d <= nms; d++) {
            if (d == 0) continue;
            int j = i + d;
            if (j >= 0 && j < n && values[j] > v) { is_peak = 0; break; }
        }
        if (!is_peak) continue;
        // enforce minimum spacing from previously accepted peaks
        if (count > 0 && (i - out_indices[count - 1]) < min_spacing) 
        {
            // keep the stronger between the two close peaks
            if (v > values[out_indices[count - 1]]) {
                out_indices[count - 1] = i;
            }
            continue;
        }
        out_indices[count++] = i;
        if (count >= max_out) break;
    }
    return count;
}


static int* smooth_projection_1d(const int* src, int n, int half_window)
{
    if (!src || n <= 0) return NULL;
    int* out = g_malloc0(sizeof(int) * n);
    if (!out) return NULL;
    for (int i = 0; i < n; i++) {
        long acc = 0; int cnt = 0;
        for (int d = -half_window; d <= half_window; d++) {
            int j = i + d; if (j < 0 || j >= n) continue; acc += src[j]; cnt++;
        }
        out[i] = cnt ? (int)(acc / cnt) : src[i];
    }
    return out;
}


static void filter_lines_by_length(Image* line_img, int* peaks, int* count, int vertical)
{
    if (!line_img || !peaks || !count || *count <= 1) return;
    int c = *count;
    int* lengths = g_malloc(sizeof(int) * c);
    if (!lengths) return;
    int max_len = 0;
    if (vertical) {
        for (int i = 0; i < c; i++) {
            int x = peaks[i]; if (x < 0) x = 0; if (x >= line_img->width) x = line_img->width - 1;
            int len = 0; for (int y = 0; y < line_img->height; y++) if (get_cached_pixel(line_img, x, y, 0) > 0) len++;
            lengths[i] = len; if (len > max_len) max_len = len;
        }
        int abs_slack = line_img->height / 30; if (abs_slack < 8) abs_slack = 8;
        double ratio = 0.75;
        int keep = 0;
        for (int i = 0; i < c; i++) {
            if (lengths[i] >= (int)(ratio * max_len) || (max_len - lengths[i]) <= abs_slack) peaks[keep++] = peaks[i];
        }
        *count = keep;
    } else {
        for (int i = 0; i < c; i++) {
            int y = peaks[i]; if (y < 0) y = 0; if (y >= line_img->height) y = line_img->height - 1;
            int len = 0; for (int x = 0; x < line_img->width; x++) if (get_cached_pixel(line_img, x, y, 0) > 0) len++;
            lengths[i] = len; if (len > max_len) max_len = len;
        }
        int abs_slack = line_img->width / 30; if (abs_slack < 8) abs_slack = 8;
        double ratio = 0.75;
        int keep = 0;
        for (int i = 0; i < c; i++) {
            if (lengths[i] >= (int)(ratio * max_len) || (max_len - lengths[i]) <= abs_slack) peaks[keep++] = peaks[i];
        }
        *count = keep;
    }
    g_free(lengths);
}

static Image* draw_grid_overlay(Image* base, const int* v_peaks, int v_count, const int* h_peaks, int h_count)
{
    if (!base) return NULL;
    Image* overlay = create_image(base->width, base->height, 3);
    if (!overlay) return NULL;
    for (int y = 0; y < base->height; y++) {
        for (int x = 0; x < base->width; x++) {
            for (int c = 0; c < 3; c++) {
                guchar px = get_cached_pixel(base, x, y, c);
                set_cached_pixel(overlay, x, y, c, px);
            }
        }
    }
    for (int i = 0; i < v_count; i++) {
        int x = v_peaks[i]; if (x < 0 || x >= overlay->width) continue;
        for (int y = 0; y < overlay->height; y++) {
            set_cached_pixel(overlay, x, y, 0, 0);
            set_cached_pixel(overlay, x, y, 1, 0);
            set_cached_pixel(overlay, x, y, 2, 255);
        }
    }
    for (int i = 0; i < h_count; i++) {
        int y = h_peaks[i]; if (y < 0 || y >= overlay->height) continue;
        for (int x = 0; x < overlay->width; x++) {
            set_cached_pixel(overlay, x, y, 0, 255);
            set_cached_pixel(overlay, x, y, 1, 0);
            set_cached_pixel(overlay, x, y, 2, 0);
        }
    }
    return overlay;
}

// Compute grid lines from vertical and horizontal lines intersection
GridLines compute_grid_lines(Image* vert_lines, Image* hori_lines, int kernel_w, int kernel_h) {
    GridLines result = {0};
    int max_lines = 2048;

    int* col_sum = vert_lines ? column_projection(vert_lines) : NULL;
    int* row_sum = hori_lines ? row_projection(hori_lines) : NULL;
    if (col_sum && vert_lines) { int* sm = smooth_projection_1d(col_sum, vert_lines->width, 2); g_free(col_sum); col_sum = sm; }
    if (row_sum && hori_lines) { int* sm = smooth_projection_1d(row_sum, hori_lines->height, 2); g_free(row_sum); row_sum = sm; }

    // --- Early exit if projections are all zero (image is blank and has no lines) ---
    int col_total = 0;
    if (col_sum && vert_lines) {
        for (int i = 0; i < vert_lines->width; ++i) col_total += col_sum[i];
    }
    int row_total = 0;
    if (row_sum && hori_lines) {
        for (int i = 0; i < hori_lines->height; ++i) row_total += row_sum[i];
    }
    if ((vert_lines && (!col_sum || col_total == 0)) && (hori_lines && (!row_sum || row_total == 0))) {
        printf("No lines found: projections are all zero.\n");
        if (col_sum) g_free(col_sum);
        if (row_sum) g_free(row_sum);
        GridLines empty = {0};
        return empty;
    }

    int* v_peaks = g_malloc0(sizeof(int) * max_lines);
    int* h_peaks = g_malloc0(sizeof(int) * max_lines);
    int v_count = 0, h_count = 0;

    if (col_sum) {
        int v_thr_raw = profile_auto_threshold(col_sum, vert_lines->width);
        int v_min_spacing = (kernel_w > 2) ? (kernel_w * 5 / 10) : 4;
        v_count = find_peaks(col_sum, vert_lines->width, v_min_spacing, v_thr_raw, v_peaks, max_lines);
    }
    if (row_sum) {
        int h_thr_raw = profile_auto_threshold(row_sum, hori_lines->height);
        int h_min_spacing = (kernel_h > 2) ? (kernel_h * 3 / 10) : 2; // 30% of est cell
        if (h_min_spacing < 2) h_min_spacing = 2;
        h_count = find_peaks(row_sum, hori_lines->height, h_min_spacing, h_thr_raw, h_peaks, max_lines);
    }

    // Length-based filtering: keep lines close to the longest line length (grid lines share similar lengths)
    if (v_count > 1 && vert_lines) { filter_lines_by_length(vert_lines, v_peaks, &v_count, 1); }
    if (h_count > 1 && hori_lines) { filter_lines_by_length(hori_lines, h_peaks, &h_count, 0); }

    // Assign to result
    result.v_peaks = v_peaks; result.v_count = v_count;
    result.h_peaks = h_peaks; result.h_count = h_count;

    if (col_sum) g_free(col_sum);
    if (row_sum) g_free(row_sum);
    return result;
}

// Image processing functions : Grayscale
Image* convert_to_grayscale(Image* img) {
    if (!img || !img->pixbuf) return NULL;
    
    Image* gray = create_image(img->width, img->height, 1);
    if (!gray) return NULL;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            // Use cached pixel access for better performance
            guchar r = get_cached_pixel(img, x, y, 0);
            guchar g = get_cached_pixel(img, x, y, 1);
            guchar b = get_cached_pixel(img, x, y, 2);
            
            // Convert to grayscale using standard luminance weights
            guchar gray_value = (guchar)(0.299 * r + 0.587 * g + 0.114 * b);
            set_cached_pixel(gray, x, y, 0, gray_value);
            // Grayscale image has only 1 channel, no need to set others
        }
    }
    printf("Step 1: Grayscale conversion\n");
    return gray;
}

// Gaussian Blur for noise reduction
Image* gaussian_blur(Image* img, int kernel_size) {
    if (!img || !img->pixbuf) return NULL;
    
    Image* blurred = create_image(img->width, img->height, img->channels);
    if (!blurred) return NULL;
    
    int half_kernel = kernel_size / 2;
    
    // Create Gaussian kernel
    double sigma = 0.8; 
    double *kernel = malloc(kernel_size * kernel_size * sizeof(double));
    double sum = 0.0;
    
    for (int ky = 0; ky < kernel_size; ky++) {
        for (int kx = 0; kx < kernel_size; kx++) {
            int x = kx - half_kernel;
            int y = ky - half_kernel;
            kernel[ky * kernel_size + kx] = exp(-(x*x + y*y) / (2.0 * sigma * sigma));
            sum += kernel[ky * kernel_size + kx];
        }
    }
    
    // Normalize kernel
    for (int ky = 0; ky < kernel_size; ky++) {
        for (int kx = 0; kx < kernel_size; kx++) {
            kernel[ky * kernel_size + kx] /= sum;
        }
    }
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) 
        {
            double gray_sum = 0;
            for (int ky = 0; ky < kernel_size; ky++) 
            {
                for (int kx = 0; kx < kernel_size; kx++) 
                {
                    int nx = x + kx - half_kernel;
                    int ny = y + ky - half_kernel;
                    
                    if (nx >= 0 && nx < img->width && ny >= 0 && ny < img->height) // Check if the pixel is within the image
                    {
                        guchar r = get_cached_pixel(img, nx, ny, 0);
                        // For grayscale images, all channels have the same value
                        double weight = kernel[ky * kernel_size + kx];
                        gray_sum += r * weight;
                    }
                }
            }
            // Set all channels to the same grayscale value
            guchar gray_value = (guchar)gray_sum;
            set_cached_pixel(blurred, x, y, 0, gray_value);
        }
    }
    // Free the kernel memory
    free(kernel);
    printf("Step 2: Gaussian blur (kernel size: %d)\n", kernel_size);
    return blurred;
}

Image* canny_edge_detection(Image* img, int low_threshold, int high_threshold) {
    if (!img || !img->pixbuf) return NULL;
    
    gint width = img->width;
    gint height = img->height;
    
    // Allocate gradient arrays
    double *gradient_mag = g_malloc(width * height * sizeof(double));
    double *gradient_dir = g_malloc(width * height * sizeof(double));
    
    if (!gradient_mag || !gradient_dir) {
        g_free(gradient_mag);
        g_free(gradient_dir);
        return NULL;
    }
    
    memset(gradient_mag, 0, width * height * sizeof(double));
    memset(gradient_dir, 0, width * height * sizeof(double));
    
    // Calculate gradients using 3x3 Sobel kernels
    for (gint y = 1; y < height - 1; y++) {
        for (gint x = 1; x < width - 1; x++) 
        {
            gint gx = 0, gy = 0;
            gx = -get_cached_pixel(img, x-1, y-1, 0) + get_cached_pixel(img, x+1, y-1, 0)
                 -2*get_cached_pixel(img, x-1, y, 0) + 2*get_cached_pixel(img, x+1, y, 0)
                 -get_cached_pixel(img, x-1, y+1, 0) + get_cached_pixel(img, x+1, y+1, 0);
        
            gy = -get_cached_pixel(img, x-1, y-1, 0) - 2*get_cached_pixel(img, x, y-1, 0) - get_cached_pixel(img, x+1, y-1, 0)
            +get_cached_pixel(img, x-1, y+1, 0) + 2*get_cached_pixel(img, x, y+1, 0) + get_cached_pixel(img, x+1, y+1, 0);
        
            gint idx = y * width + x;
            gradient_mag[idx] = sqrt(gx*gx + gy*gy);
            gradient_dir[idx] = atan2(gy, gx);
        }
    }
    // Step 2: Non-maximum suppression
    Image* edges = create_image(width, height, 1);
    if (!edges) {
        g_free(gradient_mag);
        g_free(gradient_dir);
        return NULL;
    }
    
    for (gint y = 1; y < height - 1; y++) {
        for (gint x = 1; x < width - 1; x++) {
            gint idx = y * width + x;
            double mag = gradient_mag[idx];
            double angle = gradient_dir[idx] * 180.0 / G_PI;
            
            while (angle < 0) angle += 180;
            while (angle >= 180) angle -= 180;
            
            double mag1 = 0, mag2 = 0;
            
            if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle < 180)) {
                mag1 = gradient_mag[y * width + (x-1)];
                mag2 = gradient_mag[y * width + (x+1)];
            }
            else if (angle >= 22.5 && angle < 67.5) {
                mag1 = gradient_mag[(y-1) * width + (x+1)];
                mag2 = gradient_mag[(y+1) * width + (x-1)];
            }
            else if (angle >= 67.5 && angle < 112.5) {
                mag1 = gradient_mag[(y-1) * width + x];
                mag2 = gradient_mag[(y+1) * width + x];
            }
            else {
                mag1 = gradient_mag[(y-1) * width + (x-1)];
                mag2 = gradient_mag[(y+1) * width + (x+1)];
            }
            
            if (mag >= mag1 && mag >= mag2 && mag >= high_threshold) {
                set_cached_pixel(edges, x, y, 0, 255);
            }
            else if (mag >= mag1 && mag >= mag2 && mag >= low_threshold) {
                set_cached_pixel(edges, x, y, 0, 128);
            }
            else {
                set_cached_pixel(edges, x, y, 0, 0);
            }   
        }
    }
    // Hysteresis
    typedef struct { gint x, y; } GPoint;
    GArray* stack = g_array_new(FALSE, FALSE, sizeof(GPoint));
    
    // Push all strong edges first
    for (gint y = 1; y < height - 1; y++) {
        for (gint x = 1; x < width - 1; x++) {
            if (get_cached_pixel(edges, x, y, 0) == 255) {
                GPoint p = {x, y};
                g_array_append_val(stack, p);
            }
        }
    }
    // Propagate strong edges
    while (stack->len > 0) 
    {
        GPoint p = g_array_index(stack, GPoint, stack->len - 1);
        g_array_remove_index(stack, stack->len - 1);
        for (gint dy = -1; dy <= 1; dy++) {
            for (gint dx = -1; dx <= 1; dx++) {
                gint nx = p.x + dx, ny = p.y + dy;
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                if (get_cached_pixel(edges, nx, ny, 0) == 128) {
                    set_cached_pixel(edges, nx, ny, 0, 255);
                    GPoint np = {nx, ny};
                    g_array_append_val(stack, np);
                }
            }
        }
    }
    
    // Free the GArray
    g_array_free(stack, TRUE);
    // Remove remaining weak edges
    for (gint y = 0; y < height; y++) {
        for (gint x = 0; x < width; x++) {
            if (get_cached_pixel(edges, x, y, 0) == 128) {
                set_cached_pixel(edges, x, y, 0, 0);
            }
        }
    }
    save_image("step3a_canny_edges.png", edges);
    // Morphological closing to connect broken grid lines
    Image* temp_closing = create_image(width, height, 1);
    if (temp_closing) {
        // 5x5 structuring (radius = 2)
        const gint r = 2;
        // Dilation: Expand edges to fill gaps
        for (gint y = r; y < height - r; y++) {
            for (gint x = r; x < width - r; x++) {
                guchar max_val = 0;
                for (gint dy = -r; dy <= r; dy++) {
                    for (gint dx = -r; dx <= r; dx++) {
                        guchar val = get_cached_pixel(edges, x + dx, y + dy, 0);
                        if (val > max_val) max_val = val;
                    }
                }
                set_cached_pixel(temp_closing, x, y, 0, max_val);
            }
        }
        // Erosion: Restore approximate line thickness
        for (gint y = r; y < height - r; y++) {
            for (gint x = r; x < width - r; x++) {
                guchar min_val = 255;
                for (gint dy = -r; dy <= r; dy++) {
                    for (gint dx = -r; dx <= r; dx++) {
                        guchar val = get_cached_pixel(temp_closing, x + dx, y + dy, 0);
                        if (val < min_val) min_val = val;
                    }
                }
                set_cached_pixel(edges, x, y, 0, min_val);
            }
        }
        free_image(temp_closing);
    }
    g_free(gradient_mag);
    g_free(gradient_dir);
    printf("Canny edge (thresholds: %d, %d)\n", low_threshold, high_threshold);
    
    return edges;
}


// Find contours  Note: Contour and ContourList types are defined in detection.h
ContourList* find_contours(Image* img) {
    if (!img || !img->pixbuf) return NULL;
    
    ContourList* contour_list = malloc(sizeof(ContourList));
    if (!contour_list) return NULL;
    
    // Initialize properly
    contour_list->contours = malloc(10 * sizeof(Contour));
    contour_list->count = 0;
    contour_list->capacity = 10;
    
    if (!contour_list->contours) {
        free(contour_list);
        return NULL;
    }
    
    int *visited = calloc(img->width * img->height, sizeof(int));
    if (!visited) {
        free(contour_list->contours);
        free(contour_list);
        return NULL;
    }
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            if (visited[y * img->width + x]) continue;
            
            guchar r = get_cached_pixel(img, x, y, 0);
            
            if (r > 0) { // Edge pixel found
                // Start new contour
                if (contour_list->count >= contour_list->capacity) {
                    contour_list->capacity *= 2;
                    contour_list->contours = realloc(contour_list->contours, 
                                                   contour_list->capacity * sizeof(Contour));
                }
                
                Contour* contour = &contour_list->contours[contour_list->count];
                contour->points = malloc(1000 * sizeof(Point));
                contour->count = 0;
                contour->capacity = 1000;
                
                if (!contour->points) 
                {
                    printf("Memory allocation failed for contour points\n");
                    continue;
                }
                
                // Use flood fill to find all connected edge pixels
                int stack[50000]; // Larger stack for flood fill
                int stack_top = 0;
                stack[stack_top++] = y * img->width + x;
                
                while (stack_top > 0) {
                    int current_idx = stack[--stack_top];
                    int current_x = current_idx % img->width;
                    int current_y = current_idx / img->width;
                    
                    if (visited[current_y * img->width + current_x]) continue;
                    
                    // Add to contour with dynamic reallocation
                    if (contour->count >= contour->capacity) {
                        contour->capacity *= 2;
                        Point* new_points = realloc(contour->points, contour->capacity * sizeof(Point));
                        if (!new_points) {
                            printf("Memory reallocation failed for contour points\n");
                            break;
                        }
                        contour->points = new_points;
                    }
                    
                    contour->points[contour->count].x = current_x;
                    contour->points[contour->count].y = current_y;
                    contour->count++;
                    visited[current_y * img->width + current_x] = 1;
                    
                    // Check 8-connected neighbors
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            
                            int nx = current_x + dx;
                            int ny = current_y + dy;
                            
                            if (nx >= 0 && nx < img->width && ny >= 0 && ny < img->height) {
                                int neighbor_idx = ny * img->width + nx;
                                
                                if (!visited[neighbor_idx]) {
                                    guchar nr = get_cached_pixel(img, nx, ny, 0);
                                    
                                    if (nr > 0) { // Edge pixel
                                        if (stack_top < 50000) { // Check stack bounds
                                            stack[stack_top++] = neighbor_idx;
                                        }
                                    }
                                }
                            }
                        }
                        if (stack_top >= 50000) break;
                    }
                }
                
                // Check if contour is too close to image borders (likely artificial)
                int min_x = img->width, max_x = 0, min_y = img->height, max_y = 0;
                for (int i = 0; i < contour->count; i++) {
                    if (contour->points[i].x < min_x) min_x = contour->points[i].x;
                    if (contour->points[i].x > max_x) max_x = contour->points[i].x;
                    if (contour->points[i].y < min_y) min_y = contour->points[i].y;
                    if (contour->points[i].y > max_y) max_y = contour->points[i].y;
                }
                if (contour->count > 5) { // Very low minimum to catch all contours
                    contour_list->count++;
                    printf("  Found contour with %d points\n", contour->count);
                } else {
                    free(contour->points);
                    contour->points = NULL;
                    printf("  Rejected tiny contour with %d points\n", contour->count);
                }
            }
        }
    }
    free(visited);
    printf("Contour detection completed (%d contours found)\n", contour_list->count);
    return contour_list;
}

// Find the square with the largest area
Rectangle find_largest_square(ContourList* contour_list) 
{
    Rectangle largest_rect = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, 0, 0};
    
    if (!contour_list || contour_list->count == 0) return largest_rect;
    
    double max_area = 0;
    
    for (int i = 0; i < contour_list->count; i++) {
        Contour* contour = &contour_list->contours[i];
        if (contour->count < 4) continue; // Need at least 4 points for a rectangle
        
        // Find bounding box
        int min_x = contour->points[0].x, max_x = contour->points[0].x;
        int min_y = contour->points[0].y, max_y = contour->points[0].y;
        
        for (int j = 1; j < contour->count; j++) {
            if (contour->points[j].x < min_x) min_x = contour->points[j].x;
            if (contour->points[j].x > max_x) max_x = contour->points[j].x;
            if (contour->points[j].y < min_y) min_y = contour->points[j].y;
            if (contour->points[j].y > max_y) max_y = contour->points[j].y;
        }
        
        int width = max_x - min_x;
        int height = max_y - min_y;
        double area = width * height;
        
        // Check if it's roughly square and has the largest area
        double aspect_ratio = (double)width / height;
        // More lenient aspect ratio for better detection, with minimum area
        if (aspect_ratio > 0.5 && aspect_ratio < 2.0 && area > max_area) {
            max_area = area;
            largest_rect.top_left = (Point){min_x, min_y};
            largest_rect.top_right = (Point){max_x, min_y};
            largest_rect.bottom_left = (Point){min_x, max_y};
            largest_rect.bottom_right = (Point){max_x, max_y};
            largest_rect.width = width;
            largest_rect.height = height;
        }
    }
    
    printf("✓ Step 6: Largest square detection completed (area: %.0f, contours processed: %d)\n", 
           max_area, contour_list->count);

    if (max_area == 0) {
        printf("  WARNING: No suitable square found among %d contours!\n", contour_list->count);
    }   
    return largest_rect;
}

// Compute grid rectangle from detected line peaks
Rectangle compute_grid_bounds_from_peaks(int* v_peaks, int v_count, int* h_peaks, int h_count, int img_w, int img_h)
{
    Rectangle r = {{0,0},{0,0},{0,0},{0,0},0,0};
    if (!v_peaks || !h_peaks || v_count < 2 || h_count < 2) return r;
    // Find min/max without modifying the original arrays
    int min_x = v_peaks[0], max_x = v_peaks[0];
    for (int i = 1; i < v_count; i++) { if (v_peaks[i] < min_x) min_x = v_peaks[i]; if (v_peaks[i] > max_x) max_x = v_peaks[i]; }
    int min_y = h_peaks[0], max_y = h_peaks[0];
    for (int i = 1; i < h_count; i++) { if (h_peaks[i] < min_y) min_y = h_peaks[i]; if (h_peaks[i] > max_y) max_y = h_peaks[i]; }

    if (min_x < 0) min_x = 0;
    if (max_x >= img_w) max_x = img_w - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= img_h) max_y = img_h - 1;
    if (max_x <= min_x || max_y <= min_y) return r;

    r.top_left = (Point){min_x, min_y};
    r.top_right = (Point){max_x, min_y};
    r.bottom_left = (Point){min_x, max_y};
    r.bottom_right = (Point){max_x, max_y};
    r.width = max_x - min_x;
    r.height = max_y - min_y;
    return r;
}

// Function to detect grid intersections from vertical and horizontal lines
IntersectionList* detect_grid_intersections(Image* vert_lines, Image* hori_lines, 
    int* v_peaks, int v_count, 
    int* h_peaks, int h_count) 
{
    if (!vert_lines || !hori_lines || !v_peaks || !h_peaks || v_count == 0 || h_count == 0) {
        return NULL;
    }

    IntersectionList* intersections = malloc(sizeof(IntersectionList));
    if (!intersections) return NULL;

    intersections->points = malloc(v_count * h_count * sizeof(Point));
    intersections->count = 0;
    intersections->v_lines = v_count;
    intersections->h_lines = h_count;

    if (!intersections->points) {
        free(intersections);
        return NULL;
    }

    // Search radius for finding best intersection point
    const int search_radius = 3;
    // For each vertical and horizontal line pair, find their intersection
    for (int j = 0; j < h_count; j++) {
        int h_line_y = h_peaks[j];
        for (int i = 0; i < v_count; i++) {
            int v_line_x = v_peaks[i];
            int best_x = v_line_x;
            int best_y = h_line_y;
            int best_score = 0;

            // Search in neighborhood around theoretical intersection
            for (int dy = -search_radius; dy <= search_radius; dy++) {
                for (int dx = -search_radius; dx <= search_radius; dx++) {
                    int x = v_line_x + dx;
                    int y = h_line_y + dy;
                    
                    if (x < 0 || x >= vert_lines->width || y < 0 || y >= vert_lines->height) {
                        continue;
                    }
                    
                    // Check vertical line presence (check a small vertical segment)
                    int vert_score = 0;
                    int vert_count = 0;
                    for (int vy = y - 2; vy <= y + 2; vy++) {
                        if (vy >= 0 && vy < vert_lines->height) {
                            guchar val = get_cached_pixel(vert_lines, x, vy, 0);
                            vert_score += val;
                            vert_count++;
                        }
                    }
                    if (vert_count > 0) vert_score = vert_score / vert_count;
                    
                    // Check horizontal line presence (check a small horizontal segment)
                    int hori_score = 0;
                    int hori_count = 0;
                    for (int hx = x - 2; hx <= x + 2; hx++) {
                        if (hx >= 0 && hx < hori_lines->width) {
                            guchar val = get_cached_pixel(hori_lines, hx, y, 0);
                            hori_score += val;
                            hori_count++;
                        }
                    }
                    if (hori_count > 0) hori_score = hori_score / hori_count;
                    
                    // Combined score: both lines must be present
                    int score = (vert_score + hori_score) / 2;
                    
                    // Prefer points closer to the theoretical intersection
                    int dx_abs = (dx < 0) ? -dx : dx;
                    int dy_abs = (dy < 0) ? -dy : dy;
                    int distance_penalty = dx_abs + dy_abs;
                    score = score - distance_penalty * 5; // Small penalty for distance
                    
                    if (score > best_score && vert_score > 30 && hori_score > 30) {
                        best_score = score;
                        best_x = x;
                        best_y = y;
                    }
                }
            }

            // Add intersection if we found a good match
            if (best_score > 50) {
                Point intersection = {best_x, best_y};
                intersections->points[intersections->count++] = intersection;
            }
        }
    }

    printf("Detected %d grid intersections (%d vertical x %d horizontal lines)\n", 
           intersections->count, v_count, h_count);

    return intersections;
}

static IntersectionList* intersections_from_peaks(int* v_peaks, int v_count, int* h_peaks, int h_count, int img_w, int img_h)
{
    if (!v_peaks || !h_peaks || v_count <= 0 || h_count <= 0) return NULL;
    IntersectionList* list = malloc(sizeof(IntersectionList));
    if (!list) return NULL;
    list->v_lines = v_count;
    list->h_lines = h_count;
    list->count = 0;
    size_t capacity = (size_t)v_count * (size_t)h_count;
    list->points = malloc(sizeof(Point) * capacity);
    if (!list->points) { free(list); return NULL; }
    // Row-major: iterate horizontal (rows) then vertical (cols) to maintain order
    for (int j = 0; j < h_count; j++) {
        int y = h_peaks[j]; if (y < 0) y = 0; if (y >= img_h) y = img_h - 1;
        for (int i = 0; i < v_count; i++) {
            int x = v_peaks[i]; if (x < 0) x = 0; if (x >= img_w) x = img_w - 1;
            list->points[list->count++] = (Point){ x, y };
        }
    }
    return list;
}

// Function to extract individual grid cells based on intersections
GridCells* extract_grid_cells(Image* original_img, IntersectionList* intersections) 
{
    if (!original_img || !intersections || intersections->count == 0) {
        return NULL;
    }

    GridCells* grid_cells = malloc(sizeof(GridCells));
    if (!grid_cells) return NULL;

    // Calculate grid dimensions (assuming intersections form a complete grid)
    int grid_cols = intersections->v_lines;
    int grid_rows = intersections->h_lines;

    if (grid_cols < 2 || grid_rows < 2) {
        free(grid_cells);
        return NULL;
    }

    grid_cells->cells = malloc((grid_rows - 1) * (grid_cols - 1) * sizeof(GridCell));
    grid_cells->rows = grid_rows - 1;
    grid_cells->cols = grid_cols - 1;
    grid_cells->count = 0;

    if (!grid_cells->cells) {
        free(grid_cells);
        return NULL;
    }

    // Extract each cell
    for (int row = 0; row < grid_rows - 1; row++) {
        for (int col = 0; col < grid_cols - 1; col++) {
            // Get the four corners of the cell
            int top_left_idx = row * grid_cols + col;
            int top_right_idx = row * grid_cols + (col + 1);
            int bottom_left_idx = (row + 1) * grid_cols + col;
            int bottom_right_idx = (row + 1) * grid_cols + (col + 1);

            if (top_left_idx >= intersections->count || 
                top_right_idx >= intersections->count ||
                bottom_left_idx >= intersections->count ||
                bottom_right_idx >= intersections->count) {
                printf("  Skipping cell (%d,%d): index out of bounds\n", row, col);
                continue;
            }

            Point top_left = intersections->points[top_left_idx];
            Point top_right = intersections->points[top_right_idx];
            Point bottom_left = intersections->points[bottom_left_idx];
            Point bottom_right = intersections->points[bottom_right_idx];

            // Calculate cell boundaries with some padding
            int left = top_left.x + 2;    // Small padding to avoid grid lines
            int right = top_right.x - 2;
            int top = top_left.y + 2;
            int bottom = bottom_left.y - 2;

            // Ensure valid dimensions
            if (right <= left || bottom <= top || 
                left < 0 || top < 0 || right >= original_img->width || bottom >= original_img->height) {
                printf("  Skipping invalid cell (%d,%d): bounds (%d,%d) to (%d,%d)\n", 
                       row, col, left, top, right, bottom);
                continue;
            }

            int cell_width = right - left;
            int cell_height = bottom - top;

            // Create cell image
            GridCell* cell = &grid_cells->cells[grid_cells->count];
            cell->row = row;
            cell->col = col;
            cell->image = create_image(cell_width, cell_height, original_img->channels);
            cell->image_file = NULL;

            if (!cell->image) {
                continue;
            }

            // Copy the cell region from original image
            for (int y = 0; y < cell_height; y++) {
                for (int x = 0; x < cell_width; x++) {
                    int src_x = left + x;
                    int src_y = top + y;

                    for (int c = 0; c < original_img->channels; c++) {
                        guchar pixel = get_cached_pixel(original_img, src_x, src_y, c);
                        set_cached_pixel(cell->image, x, y, c, pixel);
                    }
                }
            }

            cell->bounding_box.top_left = top_left;
            cell->bounding_box.top_right = top_right;
            cell->bounding_box.bottom_left = bottom_left;
            cell->bounding_box.bottom_right = bottom_right;
            cell->bounding_box.width = cell_width;
            cell->bounding_box.height = cell_height;

            grid_cells->count++;
        }
    }
    return grid_cells;
}

// Function to save all grid cells as individual images
int save_grid_cells(GridCells* grid_cells, const char* output_dir) 
{
    if (!grid_cells || !output_dir) return 0;

    // Create output directory
    #ifdef _WIN32
    _mkdir(output_dir);
    #else
    mkdir(output_dir, 0755);
    #endif
    int saved_count = 0;
    for (int i = 0; i < grid_cells->count; i++) {
        GridCell* cell = &grid_cells->cells[i];

        if (cell->image) {
            char filename[256];
            snprintf(filename, sizeof(filename), "%s/cell_%d_%d.png", 
                     output_dir, cell->row, cell->col);

            if (save_image(filename, cell->image)) {
                saved_count++;
                if (cell->image_file) { g_free(cell->image_file); }
                cell->image_file = g_strdup(filename);
            }
        }
    }

    printf("Saved %d grid cells to directory: %s/\n", saved_count, output_dir);
    return saved_count;
}

// Function to draw intersections on an image for visualization
Image* draw_intersections(Image* img, IntersectionList* intersections) 
{
    if (!img || !intersections) return NULL;

    Image* result = create_image(img->width, img->height, img->channels);
    if (!result) return NULL;

    // Copy original image
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                guchar pixel = get_cached_pixel(img, x, y, c);
                set_cached_pixel(result, x, y, c, pixel);
            }
        }
    }

    // Draw intersection points (green circles)
    int radius = 3;
    for (int i = 0; i < intersections->count; i++) {
        Point p = intersections->points[i];

        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int nx = p.x + dx;
                int ny = p.y + dy;

                if (nx >= 0 && nx < img->width && ny >= 0 && ny < img->height) {
                    // Draw green point
                    set_cached_pixel(result, nx, ny, 0, 0);   // R
                    set_cached_pixel(result, nx, ny, 1, 255); // G  
                    set_cached_pixel(result, nx, ny, 2, 0);   // B
                }
            }
        }
    }
    return result;
}

// Free grid cells memory
void free_grid_cells(GridCells* grid_cells) 
{
    if (grid_cells) {
        for (int i = 0; i < grid_cells->count; i++) {
            if (grid_cells->cells[i].image) {
                free_image(grid_cells->cells[i].image);
            }
            if (grid_cells->cells[i].image_file) {
                g_free(grid_cells->cells[i].image_file);
            }
        }
        free(grid_cells->cells);
        free(grid_cells);
    }
}

// Main detection function
Grid* detect_wordsearch_grid(Image* img) 
{
    if (!img || !img->pixbuf) {
        printf("Invalid image provided.\n");
        return NULL;
    }
    save_image("step0_original_input.png", img);
    
    // Step 1: Grayscale
    printf("\n=== Step 1: Grayscale Conversion ===\n");
    Image* gray = convert_to_grayscale(img);
    if (!gray) {
        printf("Failed to convert to grayscale.\n");
        return NULL;
    }
    save_image("step1_grayscale.png", gray);
    // Step 2: Gaussian Blur for noise reduction
    printf("\n=== Step 2: Gaussian Blur ===\n");
    Image* blurred = gaussian_blur(gray, 5);
    if (!blurred) {
        printf("Failed to apply Gaussian blur.\n");
        free_image(gray);
        return NULL;
    }
    save_image("step2_gaussian_blur.png", blurred);

    // Step 3: Canny Edge Detection
    printf("\n=== Step 3: Canny Edge Detection ===\n");
    Image* edges = canny_edge_detection(blurred, 50, 100);
    if (!edges) {
        printf("Failed to apply Canny edge detection.\n");
        free_image(gray);
        free_image(blurred);
        return NULL;
    }
    save_image("step3_canny_edges.png", edges);

    //Morphological opening to isolate grid lines
    printf("\n=== Step 4: Morphological Opening (grid isolation) ===\n");
    int kernel_h = img->height / 12 > 3 ? img->height / 12 : 8;
    int kernel_w = img->width / 12 > 3 ? img->width / 12 : 8;
    Image* vert_lines = open_vertical_lines(edges, kernel_h * 4 / 5, 3);
    Image* hori_lines = open_horizontal_lines(edges, kernel_w * 4 / 5, 3);
     if (vert_lines) save_image("step4a_vertical_lines.png", vert_lines);
     if (hori_lines) save_image("step4b_horizontal_lines.png", hori_lines);
    
    // Projection-based grid line detection
    printf("\n=== Step 5: Projection Peaks (grid lines) ===\n");
    GridLines gl = compute_grid_lines(vert_lines, hori_lines, kernel_w, kernel_h);
    int* v_peaks = gl.v_peaks; int v_count = gl.v_count;
    int* h_peaks = gl.h_peaks; int h_count = gl.h_count;

    // Draw overlay: vertical (blue), horizontal (red)
    if (v_count > 1 && h_count > 1) {
        Image* overlay = draw_grid_overlay(img, v_peaks, v_count, h_peaks, h_count);
        if (overlay) { save_image("step5a_grid_lines_and_intersections.png", overlay); free_image(overlay); }
    }
    IntersectionList* intersections = intersections_from_peaks(gl.v_peaks, gl.v_count, gl.h_peaks, gl.h_count, img->width, img->height);

    GridCells* grid_cells = NULL;

    if (intersections && intersections->count > 0) {
        // Draw and save intersections visualization
        Image* intersections_img = draw_intersections(img, intersections);
        if (intersections_img) {
            save_image("step5b_grid_intersections.png", intersections_img);
            free_image(intersections_img);
        }

        // Extract grid cells
        grid_cells = extract_grid_cells(img, intersections);

        // Save individual cells
        if (grid_cells) {
            save_grid_cells(grid_cells, "output_dir");
        }

        free(intersections->points);
        free(intersections);
    }
    
    // Step 6: Determine grid bounds from detected lines/intersections
    printf("\n=== Step 6: Compute Grid Bounds from line intersections ===\n");
    Rectangle grid_bounds = compute_grid_bounds_from_peaks(v_peaks, v_count, h_peaks, h_count, img->width, img->height);
    if (grid_bounds.width <= 0 || grid_bounds.height <= 0) 
    {
        printf("Peaks-based bounds failed.\n");
    }

    // Create a debug image showing the detected square
    Image* debug_square = create_image(img->width, img->height, img->channels);
    if (debug_square) {
        // Copy original image
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                for (int c = 0; c < img->channels; c++) {
                    guchar pixel = get_cached_pixel(img, x, y, c);
                    set_cached_pixel(debug_square, x, y, c, pixel);
                }
            }
        }
        // Draw grid bounds outline
        for (int x = grid_bounds.top_left.x; x <= grid_bounds.top_right.x; x++) {
            // Top edge
            set_cached_pixel(debug_square, x, grid_bounds.top_left.y, 0, 255);
            if (debug_square->channels > 1) set_cached_pixel(debug_square, x, grid_bounds.top_left.y, 1, 0);
            if (debug_square->channels > 2) set_cached_pixel(debug_square, x, grid_bounds.top_left.y, 2, 0);
            // Bottom edge
            set_cached_pixel(debug_square, x, grid_bounds.bottom_left.y, 0, 255);
            if (debug_square->channels > 1) set_cached_pixel(debug_square, x, grid_bounds.bottom_left.y, 1, 0);
            if (debug_square->channels > 2) set_cached_pixel(debug_square, x, grid_bounds.bottom_left.y, 2, 0);
        }
        for (int y = grid_bounds.top_left.y; y <= grid_bounds.bottom_left.y; y++) {
            // Left edge
            set_cached_pixel(debug_square, grid_bounds.top_left.x, y, 0, 255);
            if (debug_square->channels > 1) set_cached_pixel(debug_square, grid_bounds.top_left.x, y, 1, 0);
            if (debug_square->channels > 2) set_cached_pixel(debug_square, grid_bounds.top_left.x, y, 2, 0);
            // Right edge
            set_cached_pixel(debug_square, grid_bounds.top_right.x, y, 0, 255);
            if (debug_square->channels > 1) set_cached_pixel(debug_square, grid_bounds.top_right.x, y, 1, 0);
            if (debug_square->channels > 2) set_cached_pixel(debug_square, grid_bounds.top_right.x, y, 2, 0);
        }
        
        save_image("step6_detected_square.png", debug_square);
        free_image(debug_square);
    }

    // Free morphology & projections
    if (vert_lines) free_image(vert_lines);
    if (hori_lines) free_image(hori_lines);
    if (v_peaks) g_free(v_peaks);
    if (h_peaks) g_free(h_peaks);
    
    // Clean up intermediate images
    free_image(gray);
    free_image(blurred);
    free_image(edges);  


    // Build and return Grid with 2D cells if we have extracted cells
    if (grid_cells) {
        Grid* grid = g_new0(Grid, 1);
        if (!grid) {
            // We own grid_cells here; free only the containers
            free(grid_cells->cells);
            free(grid_cells);
            return NULL;
        }
        grid->bounds = grid_bounds;
        grid->rows = grid_cells->rows;
        grid->cols = grid_cells->cols;
        grid->letters = NULL;      // populated by OCR 

        // Allocate 2D array
        grid->cells = (GridCell**)calloc(grid->rows, sizeof(GridCell*));
        if (!grid->cells) {
            free(grid);
            free(grid_cells->cells);
            free(grid_cells);
            return NULL;
        }
        for (int r = 0; r < grid->rows; r++) {
            grid->cells[r] = (GridCell*)calloc(grid->cols, sizeof(GridCell));
            if (!grid->cells[r]) {
                // Cleanup previously allocated rows
                for (int rr = 0; rr < r; rr++) free(grid->cells[rr]);
                free(grid->cells);
                free(grid);
                free(grid_cells->cells);
                free(grid_cells);
                return NULL;
            }
        }
		// Move only valid cells using recorded row/col indices
		for (int i = 0; i < grid_cells->count; i++) {
			GridCell* src = &grid_cells->cells[i];
			int r = src->row;
			int c = src->col;
			if (r < 0 || r >= grid->rows || c < 0 || c >= grid->cols) {
				continue;
			}
			GridCell* dst = &grid->cells[r][c];
			*dst = *src; // struct copy (moves image/image_file pointers)
			src->image = NULL;
			src->image_file = NULL;
		}
        // Free the temporary GridCells container (no images inside now)
        free(grid_cells->cells);
        free(grid_cells);

        return grid;
    }
    return NULL;
}

