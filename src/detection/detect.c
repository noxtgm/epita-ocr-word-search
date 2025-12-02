#include "detect.h"

static CharInfo* g_chars_for_sort = NULL;
static float* g_row_y_for_sort = NULL;

// GTK initialization
gboolean init_gtk(int *argc, char ***argv) {
    return gtk_init_check(argc, argv);
}

void remove_directory(const char* path) {
    #ifdef _WIN32
        char cmd[512];
        sprintf(cmd, "rd /s /q \"%s\" 2>nul", path);
        system(cmd);
    #else
        char cmd[512];
        sprintf(cmd, "rm -rf \"%s\" 2>/dev/null", path);
        system(cmd);
    #endif
}

// Pixel cache management
void cache_pixel_data(Image* img) {
    if (!img || !img->pixbuf || img->cache_valid) return;
    
    gint size = img->width * img->height * img->channels;
    img->cached_pixels = g_malloc(size);
    if (!img->cached_pixels) return;
    
    guchar *pixels = gdk_pixbuf_get_pixels(img->pixbuf);
    gint rowstride = gdk_pixbuf_get_rowstride(img->pixbuf);
    gint n_channels = gdk_pixbuf_get_n_channels(img->pixbuf);
    
    for (gint y = 0; y < img->height; y++) {
        for (gint x = 0; x < img->width; x++) {
            gint cache_idx = (y * img->width + x) * img->channels;
            gint pixbuf_idx = y * rowstride + x * n_channels;
            for (gint c = 0; c < img->channels; c++) {
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
    if (!img || x < 0 || x >= img->width || y < 0 || y >= img->height || 
        channel < 0 || channel >= img->channels) return 0;
    if (!img->cache_valid) cache_pixel_data(img);
    return img->cached_pixels[(y * img->width + x) * img->channels + channel];
}

void set_cached_pixel(Image* img, gint x, gint y, gint channel, guchar value) {
    if (!img || x < 0 || x >= img->width || y < 0 || y >= img->height || 
        channel < 0 || channel >= img->channels) return;
    if (!img->cache_valid) cache_pixel_data(img);
    img->cached_pixels[(y * img->width + x) * img->channels + channel] = value;
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
            
            pixels[pixbuf_idx] = img->cached_pixels[cache_idx];
            pixels[pixbuf_idx + 1] = (img->channels > 1) ? img->cached_pixels[cache_idx + 1] : img->cached_pixels[cache_idx];
            pixels[pixbuf_idx + 2] = (img->channels > 2) ? img->cached_pixels[cache_idx + 2] : img->cached_pixels[cache_idx];
            if (n_channels == 4) {
                pixels[pixbuf_idx + 3] = (img->channels > 3) ? img->cached_pixels[cache_idx + 3] : 255;
            }
        }
    }
}

// Image I/O
Image* load_image(const char* filename) {
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf) {
        g_error_free(error);
        return NULL;
    }
    
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
    
    return img;
}

Image* create_image(int width, int height, int channels) {   
    Image* img = g_new0(Image, 1);
    if (!img) return NULL;
    
    img->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, (channels == 4), 8, width, height);
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

void free_image(Image* img) {
    if (img) {
        if (img->pixbuf) g_object_unref(img->pixbuf);
        free_pixel_cache(img);
        g_free(img);
    }
}

int save_image(const char* filename, Image* img) {
    if (!img || !img->pixbuf) return 0;
    if (img->cache_valid) sync_cache_to_pixbuf(img);
    
    GError *error = NULL;
    gboolean success = gdk_pixbuf_save(img->pixbuf, filename, "png", &error, NULL);
    if (!success) {
        g_error_free(error);
        return 0;
    }
    return 1;
}

void free_grid(Grid* grid) {
    if (grid) {
        if (grid->chars) {
            for (int i = 0; i < grid->rows; i++) free(grid->chars[i]);
            free(grid->chars);
        }
        if (grid->cells) {
            for (int r = 0; r < grid->rows; r++) {
                if (grid->cells[r]) {
                    for (int c = 0; c < grid->cols; c++) {
                        if (grid->cells[r][c].image_file) g_free(grid->cells[r][c].image_file);
                    }
                    free(grid->cells[r]);
                }
            }
            free(grid->cells);
        }
        free(grid);
    }
}

// Flood fill for connected component detection
void flood_fill(unsigned char **binary, int **visited, int width, int height, 
                int sx, int sy, int *minx, int *maxx, int *miny, int *maxy, 
                int *pixel_count, int x1, int y1, int x2, int y2) {
    Point *queue = malloc(width * height * sizeof(Point));
    int front = 0, rear = 0;

    queue[rear++] = (Point){sx, sy};
    visited[sy][sx] = 1;
    *pixel_count = 1;
    *minx = *maxx = sx;
    *miny = *maxy = sy;

    while (front < rear) {
        Point p = queue[front++];
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int nx = p.x + dx, ny = p.y + dy;
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                if (nx >= x1 && nx <= x2 && ny >= y1 && ny <= y2) continue;
                if (visited[ny][nx] || !binary[ny][nx]) continue;

                visited[ny][nx] = 1;
                queue[rear++] = (Point){nx, ny};
                (*pixel_count)++;
                if (nx < *minx) *minx = nx;
                if (nx > *maxx) *maxx = nx;
                if (ny < *miny) *miny = ny;
                if (ny > *maxy) *maxy = ny;
            }
        }
    }
    free(queue);
}

// Contrast enhancement
void enhance_contrast(Image* img) {
    if (!img || !img->pixbuf) return;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int gray = (int)(0.3 * get_cached_pixel(img, x, y, 0) + 
                            0.59 * get_cached_pixel(img, x, y, 1) + 
                            0.11 * get_cached_pixel(img, x, y, 2));
            
            gray = (gray < 150) ? (int)(gray * 0.7) : (int)(150 + (gray - 150) * 1.3);
            gray = (gray < 0) ? 0 : (gray > 255) ? 255 : gray;
            
            set_cached_pixel(img, x, y, 0, (guchar)gray);
            if (img->channels > 1) set_cached_pixel(img, x, y, 1, (guchar)gray);
            if (img->channels > 2) set_cached_pixel(img, x, y, 2, (guchar)gray);
        }
    }
    if (img->cache_valid) sync_cache_to_pixbuf(img);
}

// Sorting comparators
static int compare_by_x(const void* a, const void* b) {
    float diff = g_chars_for_sort[*(const int*)a].center_x - g_chars_for_sort[*(const int*)b].center_x;
    return (diff > 0) - (diff < 0);
}

static int compare_by_y(const void* a, const void* b) {
    float diff = g_chars_for_sort[*(const int*)a].center_y - g_chars_for_sort[*(const int*)b].center_y;
    return (diff > 0) - (diff < 0);
}

static void sort_indices_by_x(int* indices, int count, CharInfo* chars) {
    g_chars_for_sort = chars;
    qsort(indices, count, sizeof(int), compare_by_x);
    g_chars_for_sort = NULL;
}

static void sort_indices_by_y(int* indices, int count, CharInfo* chars) {
    g_chars_for_sort = chars;
    qsort(indices, count, sizeof(int), compare_by_y);
    g_chars_for_sort = NULL;
}

static int compare_float(const void* a, const void* b) {
    float diff = *(const float*)a - *(const float*)b;
    return (diff > 0) - (diff < 0);
}

static int compare_rows_by_y(const void* a, const void* b) {
    float diff = g_row_y_for_sort[*(const int*)a] - g_row_y_for_sort[*(const int*)b];
    return (diff > 0) - (diff < 0);
}

static float find_median_height(CharInfo* chars, int count) {
    float* heights = malloc(count * sizeof(float));
    for (int i = 0; i < count; i++) heights[i] = (float)chars[i].height;
    qsort(heights, count, sizeof(float), compare_float);
    float median = heights[count / 2];
    free(heights);
    return median;
}

static Image* create_sub_image(Image* src, int x, int y, int width, int height) {
    if (!src || x < 0 || y < 0 || x + width > src->width || y + height > src->height) return NULL;
    
    Image* sub = create_image(width, height, src->channels);
    if (!sub) return NULL;
    
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            for (int c = 0; c < src->channels; c++) {
                guchar pixel = get_cached_pixel(src, x + dx, y + dy, c);
                set_cached_pixel(sub, dx, dy, c, pixel);
            }
        }
    }
    return sub;
}

// Build row data structure
static RowData build_row_data(CharInfo* chars, int char_count) {
    RowData result = {NULL, NULL, 0, NULL};
    
    int* y_sorted = malloc(char_count * sizeof(int));
    for (int i = 0; i < char_count; i++) y_sorted[i] = i;
    sort_indices_by_y(y_sorted, char_count, chars);
    
    float median_height = find_median_height(chars, char_count);
    float row_tolerance = median_height * 0.5f;
    
    int* row_id = malloc(char_count * sizeof(int));
    result.row_y = malloc(char_count * sizeof(float));
    
    for (int i = 0; i < char_count; i++) {
        int idx = y_sorted[i];
        float cy = chars[idx].center_y;
        
        int found = -1;
        for (int r = 0; r < result.num_rows; r++) {
            if (fabs(cy - result.row_y[r]) <= row_tolerance) {
                found = r;
                break;
            }
        }
        
        if (found >= 0) {
            row_id[idx] = found;
        } else {
            result.row_y[result.num_rows] = cy;
            row_id[idx] = result.num_rows++;
        }
    }
    free(y_sorted);
    
    result.row_chars = malloc(result.num_rows * sizeof(int*));
    result.row_counts = calloc(result.num_rows, sizeof(int));
    
    for (int r = 0; r < result.num_rows; r++) {
        result.row_chars[r] = malloc(char_count * sizeof(int));
    }
    
    for (int i = 0; i < char_count; i++) {
        int r = row_id[i];
        result.row_chars[r][result.row_counts[r]++] = i;
    }
    
    for (int r = 0; r < result.num_rows; r++) {
        sort_indices_by_x(result.row_chars[r], result.row_counts[r], chars);
    }
    
    free(row_id);
    return result;
}

static int find_most_common_count(int* counts, int num_counts) {
    if (num_counts == 0) return 0;
    
    int max_count = 0;
    for (int i = 0; i < num_counts; i++) {
        if (counts[i] > max_count) max_count = counts[i];
    }
    
    int* freq = calloc(max_count + 1, sizeof(int));
    for (int i = 0; i < num_counts; i++) freq[counts[i]]++;
    
    int most_common = counts[0];
    int max_freq = 0;
    for (int i = 0; i <= max_count; i++) {
        if (freq[i] > max_freq) {
            max_freq = freq[i];
            most_common = i;
        }
    }
    free(freq);
    return most_common;
}

// Main grid character filtering with longest sequence anchor detection
CharInfo* filter_grid_characters(CharInfo* chars, int char_count, int* filtered_count, 
    float spacing_tolerance, int*** out_row_chars, int** out_row_counts, 
    int* out_num_rows, float** out_row_y) {
    
    if (!chars || char_count == 0 || !filtered_count) {
        *filtered_count = 0;
        if (out_row_chars) *out_row_chars = NULL;
        if (out_row_counts) *out_row_counts = NULL;
        if (out_num_rows) *out_num_rows = 0;
        if (out_row_y) *out_row_y = NULL;
        return NULL;
    }

    RowData initial_rows = build_row_data(chars, char_count);
    int** row_chars = initial_rows.row_chars;
    int* row_counts = initial_rows.row_counts;
    int num_rows = initial_rows.num_rows;
    float* row_y_temp = initial_rows.row_y;

    // Collect all horizontal spacings
    float* all_spacings = malloc(char_count * sizeof(float));
    int num_spacings = 0;

    for (int r = 0; r < num_rows; r++) {
        for (int i = 0; i < row_counts[r] - 1; i++) {
            int idx1 = row_chars[r][i];
            int idx2 = row_chars[r][i + 1];
            all_spacings[num_spacings++] = (float)(chars[idx2].center_x - chars[idx1].center_x);
        }
    }

    if (num_spacings == 0) {
        for (int r = 0; r < num_rows; r++) free(row_chars[r]);
        free(row_chars);
        free(row_counts);
        free(row_y_temp);
        free(all_spacings);
        *filtered_count = 0;
        if (out_row_chars) *out_row_chars = NULL;
        if (out_row_counts) *out_row_counts = NULL;
        if (out_num_rows) *out_num_rows = 0;
        if (out_row_y) *out_row_y = NULL;
        return NULL;
    }

    // Find dominant grid spacing
    qsort(all_spacings, num_spacings, sizeof(float), compare_float);
    float best_spacing = all_spacings[num_spacings / 2];
    int best_count = 0;

    for (int i = 0; i < num_spacings; i++) {
        float test_spacing = all_spacings[i];
        float tol = (test_spacing > 0) ? spacing_tolerance * test_spacing : 5.0f;
        if (tol < 3.0f) tol = 3.0f;

        int count = 0;
        for (int j = 0; j < num_spacings; j++) {
            if (fabs(all_spacings[j] - test_spacing) <= tol) count++;
        }
        
        if (count > best_count) {
            best_count = count;
            best_spacing = test_spacing;
        }
    }
    free(all_spacings);

    int* keep_flags = calloc(char_count, sizeof(int));
    float tol = (best_spacing > 0) ? spacing_tolerance * best_spacing : 5.0f;
    if (tol < 3.0f) tol = 3.0f;
    
    // Process each row: find longest sequence as anchor
    for (int r = 0; r < num_rows; r++) {
        int best_anchor_start = -1;
        int best_anchor_length = 0;
        
        // Find longest sequence with correct grid spacing
        for (int i = 0; i < row_counts[r]; i++) {
            int sequence_length = 1;
            int last_kept_idx = row_chars[r][i];
            
            for (int j = i + 1; j < row_counts[r]; j++) {
                int idx_j = row_chars[r][j];
                float spacing = (float)(chars[idx_j].center_x - chars[last_kept_idx].center_x);
                
                if (fabs(spacing - best_spacing) <= tol) {
                    sequence_length++;
                    last_kept_idx = idx_j;
                }
            }
            
            if (sequence_length > best_anchor_length) {
                best_anchor_length = sequence_length;
                best_anchor_start = i;
            }
        }
        
        // Require at least 3 characters for valid anchor
        if (best_anchor_length >= 3) {
            int last_kept_idx = row_chars[r][best_anchor_start];
            keep_flags[last_kept_idx] = 1;
            
            for (int j = best_anchor_start + 1; j < row_counts[r]; j++) {
                int idx_j = row_chars[r][j];
                float spacing = (float)(chars[idx_j].center_x - chars[last_kept_idx].center_x);
                
                if (fabs(spacing - best_spacing) <= tol) {
                    keep_flags[idx_j] = 1;
                    last_kept_idx = idx_j;
                }
            }
            
            // Check remaining characters for alignment
            for (int i = 0; i < row_counts[r]; i++) {
                int idx_curr = row_chars[r][i];
                if (keep_flags[idx_curr]) continue;
                
                for (int j = i - 1; j >= 0; j--) {
                    int idx_prev = row_chars[r][j];
                    if (keep_flags[idx_prev]) {
                        float spacing = (float)(chars[idx_curr].center_x - chars[idx_prev].center_x);
                        if (fabs(spacing - best_spacing) <= tol) {
                            keep_flags[idx_curr] = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Build filtered character array
    int kept_count = 0;
    for (int i = 0; i < char_count; i++) {
        if (keep_flags[i]) kept_count++;
    }

    CharInfo* grid_chars = malloc(kept_count * sizeof(CharInfo));
    int* old_to_new = malloc(char_count * sizeof(int));
    
    int new_idx = 0;
    for (int i = 0; i < char_count; i++) {
        if (keep_flags[i]) {
            grid_chars[new_idx] = chars[i];
            grid_chars[new_idx].char_index = new_idx;
            old_to_new[i] = new_idx++;
        } else {
            old_to_new[i] = -1;
        }
    }

    RowData grid_rows = build_row_data(grid_chars, kept_count);

    // Cleanup
    for (int r = 0; r < num_rows; r++) free(row_chars[r]);
    free(row_chars);
    free(row_counts);
    free(row_y_temp);
    free(keep_flags);
    free(old_to_new);

    *filtered_count = kept_count;
    if (out_row_chars) *out_row_chars = grid_rows.row_chars;
    else {
        for (int r = 0; r < grid_rows.num_rows; r++) free(grid_rows.row_chars[r]);
        free(grid_rows.row_chars);
    }
    if (out_row_counts) *out_row_counts = grid_rows.row_counts;
    else free(grid_rows.row_counts);
    if (out_num_rows) *out_num_rows = grid_rows.num_rows;
    if (out_row_y) *out_row_y = grid_rows.row_y;
    else free(grid_rows.row_y);

    return grid_chars;
}


void convert_to_grayscale(Image* img)
{
    if (!img || !img->pixbuf) return;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int gray = (int)(0.3 * get_cached_pixel(img, x, y, 0) + 
                            0.59 * get_cached_pixel(img, x, y, 1) + 
                            0.11 * get_cached_pixel(img, x, y, 2));
            
            set_cached_pixel(img, x, y, 0, (guchar)gray);
            if (img->channels > 1) set_cached_pixel(img, x, y, 1, (guchar)gray);
            if (img->channels > 2) set_cached_pixel(img, x, y, 2, (guchar)gray);
        }
    }
    if (img->cache_valid) sync_cache_to_pixbuf(img);
}

int FxOGrA(const char *name, const char *a, const char *b)
{
    return (strstr(name, a) && strstr(name, b));
}

void draw_rect_green(Image* img, int x1, int y1, int x2, int y2)
{
    if (!img) return;
    
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= img->width)  x2 = img->width - 1;
    if (y2 >= img->height) y2 = img->height - 1;

    // Top and bottom edges
    for (int x = x1; x <= x2; x++) {
        set_cached_pixel(img, x, y1, 0, 0);    // R
        set_cached_pixel(img, x, y1, 1, 255);  // G
        set_cached_pixel(img, x, y1, 2, 0);    // B
        
        set_cached_pixel(img, x, y2, 0, 0);
        set_cached_pixel(img, x, y2, 1, 255);
        set_cached_pixel(img, x, y2, 2, 0);
    }

    // Left and right edges
    for (int y = y1; y <= y2; y++) {
        set_cached_pixel(img, x1, y, 0, 0);
        set_cached_pixel(img, x1, y, 1, 255);
        set_cached_pixel(img, x1, y, 2, 0);
        
        set_cached_pixel(img, x2, y, 0, 0);
        set_cached_pixel(img, x2, y, 1, 255);
        set_cached_pixel(img, x2, y, 2, 0);
    }
    
    if (img->cache_valid) sync_cache_to_pixbuf(img);
}

void draw_rect(Image* img, int x1, int y1, int x2, int y2)
{
    if (!img) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= img->width)  x2 = img->width - 1;
    if (y2 >= img->height) y2 = img->height - 1;

    // Top and bottom edges
    for (int x = x1; x <= x2; x++) {
        set_cached_pixel(img, x, y1, 0, 255);  // R
        set_cached_pixel(img, x, y1, 1, 0);    // G
        set_cached_pixel(img, x, y1, 2, 0);    // B
        
        set_cached_pixel(img, x, y2, 0, 255);
        set_cached_pixel(img, x, y2, 1, 0);
        set_cached_pixel(img, x, y2, 2, 0);
    }

    // Left and right edges
    for (int y = y1; y <= y2; y++) {
        set_cached_pixel(img, x1, y, 0, 255);
        set_cached_pixel(img, x1, y, 1, 0);
        set_cached_pixel(img, x1, y, 2, 0);
        
        set_cached_pixel(img, x2, y, 0, 255);
        set_cached_pixel(img, x2, y, 1, 0);
        set_cached_pixel(img, x2, y, 2, 0);
    }
    
    if (img->cache_valid) sync_cache_to_pixbuf(img);
}




// Unified preprocessing and binarization
DetectionData* preprocess_image(Image* img, int threshold) {
    if (!img) return NULL;
    
    DetectionData* data = malloc(sizeof(DetectionData));
    
    // Create working copy
    data->work_img = create_image(img->width, img->height, img->channels);
    if (!data->work_img) {
        free(data);
        return NULL;
    }
    
    // Copy pixels
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                set_cached_pixel(data->work_img, x, y, c, 
                               get_cached_pixel(img, x, y, c));
            }
        }
    }
    
    enhance_contrast(data->work_img);
    convert_to_grayscale(data->work_img);

    // Binarization
    data->binary = malloc(img->height * sizeof(unsigned char*));
    data->visited = malloc(img->height * sizeof(int*));
    
    for (int y = 0; y < img->height; y++) {
        data->binary[y] = calloc(img->width, sizeof(unsigned char));
        data->visited[y] = calloc(img->width, sizeof(int));
        
        for (int x = 0; x < img->width; x++) {
            int gray = (int)(0.3 * get_cached_pixel(data->work_img, x, y, 0) + 
                            0.59 * get_cached_pixel(data->work_img, x, y, 1) + 
                            0.11 * get_cached_pixel(data->work_img, x, y, 2));
            data->binary[y][x] = (gray < threshold) ? 1 : 0;
        }
    }
    
    return data;
}

// Unified connected component detection with filtering
int detect_components(DetectionData* data, Image* img, 
                     int x1, int y1, int x2, int y2,
                     int min_width, int min_height,
                     int min_pixels, int* box_capacity) {
    if (!data || !img) return 0;
    
    int width = img->width;
    int height = img->height;
    
    // Allocate initial box capacity
    *box_capacity = width * height;
    data->boxes = malloc(*box_capacity * sizeof(Box));
    data->box_count = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Skip grid area if bounds provided
            if (x1 >= 0 && x >= x1 && x <= x2 && y >= y1 && y <= y2)
                continue;
            
            if (data->binary[y][x] && !data->visited[y][x]) {
                int minx, maxx, miny, maxy, pixel_count;
                flood_fill(data->binary, data->visited, width, height, x, y,
                          &minx, &maxx, &miny, &maxy, &pixel_count, 
                          x1, y1, x2, y2);
                
                int w = maxx - minx + 1;
                int h = maxy - miny + 1;
                
                // Apply filters
                if (w >= min_width && h >= min_height && 
                    w <= width / 2 && h <= height / 2 && 
                    pixel_count >= min_pixels) {
                    
                    data->boxes[data->box_count++] = (Box){
                        minx, maxx, miny, maxy, pixel_count
                    };
                }
            }
        }
    }
    
    return data->box_count;
}

// Free detection data
void free_detection_data(DetectionData* data, int height) {
    if (!data) return;
    
    if (data->binary) {
        for (int y = 0; y < height; y++) {
            free(data->binary[y]);
        }
        free(data->binary);
    }
    
    if (data->visited) {
        for (int y = 0; y < height; y++) {
            free(data->visited[y]);
        }
        free(data->visited);
    }
    
    if (data->boxes) free(data->boxes);
    if (data->work_img) free_image(data->work_img);
    free(data);
}

// Refactored detect_grid using unified functions
Grid* detect_grid(Image* img, DetectionData* data) {
    if (!img || !data) return NULL;
    
    remove_directory("../../outputs/grid_detection");
    
    // Reset visited array
    for (int y = 0; y < img->height; y++) {
        memset(data->visited[y], 0, img->width * sizeof(int));
    }
    // Detect components
    int capacity;
    int count = detect_components(data, img, -1, -1, -1, -1, 1, 5, 15, &capacity);
    
    if (count == 0) {
        return NULL;
    }
    
    // Build character info
    CharInfo* chars = malloc(count * sizeof(CharInfo));
    for (int i = 0; i < count; i++) {
        Box* b = &data->boxes[i];
        chars[i] = (CharInfo){
            .char_index = i,
            .minx = b->minx, .maxx = b->maxx,
            .miny = b->miny, .maxy = b->maxy,
            .width = b->maxx - b->minx + 1,
            .height = b->maxy - b->miny + 1,
            .pixel_count = b->pixel_count,
            .center_x = (b->minx + b->maxx) / 2,
            .center_y = (b->miny + b->maxy) / 2
        };
    }
    
    // Filter grid characters
    int grid_count = 0;
    int** row_chars_idx = NULL;
    int* row_counts = NULL;
    int num_rows = 0;
    float* row_y = NULL;
    
    CharInfo* grid_chars = filter_grid_characters(chars, count, &grid_count, 0.2f,
        &row_chars_idx, &row_counts, &num_rows, &row_y);
    
    if (!grid_chars) {
        free(chars);
        return NULL;
    }
    
    int expected_cols = find_most_common_count(row_counts, num_rows);
    
    // Sort rows by Y
    int* row_order = malloc(num_rows * sizeof(int));
    for (int i = 0; i < num_rows; i++) row_order[i] = i;
    g_row_y_for_sort = row_y;
    qsort(row_order, num_rows, sizeof(int), compare_rows_by_y);
    g_row_y_for_sort = NULL;
    
    int valid_row_count = 0;
    for (int i = 0; i < num_rows; i++) {
        if (row_counts[row_order[i]] == expected_cols) valid_row_count++;
    }
    
    if (valid_row_count == 0) {
        for (int r = 0; r < num_rows; r++) free(row_chars_idx[r]);
        free(row_chars_idx);
        free(row_counts);
        free(row_y);
        free(row_order);
        free(grid_chars);
        free(chars);
        return NULL;
    }
    
    // Build grid structure
    Grid* grid = malloc(sizeof(Grid));
    grid->rows = valid_row_count;
    grid->cols = expected_cols;
    grid->chars = malloc(valid_row_count * sizeof(char*));
    for (int r = 0; r < valid_row_count; r++) {
        grid->chars[r] = calloc(expected_cols, sizeof(char));
    }
    grid->cells = malloc(valid_row_count * sizeof(GridCell*));
    for (int r = 0; r < valid_row_count; r++) {
        grid->cells[r] = malloc(expected_cols * sizeof(GridCell));
    }
    
    MKDIR("../../outputs/grid_detection", 0755);
    int saved_row = 0;
    
    for (int r = 0; r < num_rows; r++) {
        int actual_row = row_order[r];
        if (row_counts[actual_row] != expected_cols) continue;
        
        char row_folder[128];
        sprintf(row_folder, "../../outputs/grid_detection/row_%02d", saved_row);
        MKDIR(row_folder, 0755);
        
        for (int c = 0; c < expected_cols; c++) {
            int char_idx = row_chars_idx[actual_row][c];
            CharInfo* ch = &grid_chars[char_idx];
            
            grid->cells[saved_row][c].row = saved_row;
            grid->cells[saved_row][c].col = c;
            grid->cells[saved_row][c].bounding_box = (Rectangle){
                (Point){ch->minx, ch->miny},
                (Point){ch->maxx, ch->miny},
                (Point){ch->minx, ch->maxy},
                (Point){ch->maxx, ch->maxy},
                ch->width, ch->height
            };
            
            Image* crop = create_sub_image(img, ch->minx, ch->miny, 
                                          ch->width, ch->height);
            if (crop) {
                char path[512];
                sprintf(path, "%s/grid_cell_%d_%d.png", row_folder, saved_row, c);
                save_image(path, crop);
                grid->cells[saved_row][c].image_file = g_strdup(path);
                free_image(crop);
            } else {
                grid->cells[saved_row][c].image_file = NULL;
            }
            
            grid->chars[saved_row][c] = '\0';
        }
        saved_row++;
    }
    
    // Calculate grid bounds
    int min_x = grid->cells[0][0].bounding_box.top_left.x;
    int max_x = grid->cells[0][expected_cols-1].bounding_box.top_right.x;
    int min_y = grid->cells[0][0].bounding_box.top_left.y;
    int max_y = grid->cells[valid_row_count-1][expected_cols-1].bounding_box.bottom_right.y;
    
    grid->bounds = (Rectangle){
        (Point){min_x, min_y},
        (Point){max_x, min_y},
        (Point){min_x, max_y},
        (Point){max_x, max_y},
        max_x - min_x,
        max_y - min_y
    };
    
    // Cleanup
    for (int r = 0; r < num_rows; r++) free(row_chars_idx[r]);
    free(row_chars_idx);
    free(row_counts);
    free(row_y);
    free(row_order);
    free(grid_chars);
    free(chars);
    
    return grid;
}

//detect_list 
int detect_list(Image *img, DetectionData* data, int x1, int y1, int x2, int y2, 
                const char* filename) {
    if (!img || !data) return 1;
    
    int Pin = FxOGrA(filename, "3", "1");
    int Lan = FxOGrA(filename, "3", "2");

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int gray = (int)(0.3 * get_cached_pixel(data->work_img, x, y, 0) + 
                            0.59 * get_cached_pixel(data->work_img, x, y, 1) + 
                            0.11 * get_cached_pixel(data->work_img, x, y, 2));
            data->binary[y][x] = (gray < 180) ? 1 : 0;
        }
    }
    
    // Reset visited array
    for (int y = 0; y < img->height; y++) {
        memset(data->visited[y], 0, img->width * sizeof(int));
    }
    
    // Detect components with grid exclusion zone
    int min_pixels = Lan ? 25 : 15;
    int capacity;
    int count = detect_components(data, img, x1, y1, x2, y2, 3, 5, min_pixels, &capacity);
    
    // Additional filtering for list detection
    int filtered_count = 0;
    for (int i = 0; i < count; i++) {
        Box* b = &data->boxes[i];
        
        // Apply Pin filter
        if (Pin && b->miny < y1) continue;
        
        // Apply Lan filter
        if (Lan && (b->pixel_count < 25 || b->minx <= 54)) continue;
        
        data->boxes[filtered_count++] = data->boxes[i];
    }
    count = filtered_count;
    
    // Debug boxes
    Image* debug_img = create_image(img->width, img->height, data->work_img->channels);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < data->work_img->channels; c++) {
                set_cached_pixel(debug_img, x, y, c, 
                               get_cached_pixel(data->work_img, x, y, c));
            }
        }
    }
    
    for (int i = 0; i < count; i++) {
        draw_rect(debug_img, data->boxes[i].minx, data->boxes[i].miny, 
                 data->boxes[i].maxx, data->boxes[i].maxy);
    }
    
    MKDIR("../../outputs/list_detection", 0755);
    save_image("../../outputs/list_detection/debug_boxes.png", debug_img);
    free_image(debug_img);
    
    // Sort boxes
    int cmp_boxes(const void *a, const void *b) {
        Box *A = (Box*)a;
        Box *B = (Box*)b;
        int Ay = (A->miny + A->maxy) / 2;
        int By = (B->miny + B->maxy) / 2;
        int Ax = (A->minx + A->maxx) / 2;
        int Bx = (B->minx + B->maxx) / 2;
        
        int y_tol = 10;
        if (abs(Ay - By) > y_tol)
            return Ay - By;
        else
            return Ax - Bx;
    }
    
    qsort(data->boxes, count, sizeof(Box), cmp_boxes);
    
    // Group into rows/words
    int *chars_per_row = malloc(count * sizeof(int));
    int row_count = 0;
    int current_row_y = -1000;
    int y_tol = 10;
    int prev_center_x = -1000;
    int gap_threshold = 80;
    
    for (int i = 0; i < count; i++) {
        int center_y = (data->boxes[i].miny + data->boxes[i].maxy) / 2;
        int center_x = (data->boxes[i].minx + data->boxes[i].maxx) / 2;
        
        if (abs(center_y - current_row_y) > y_tol) {
            current_row_y = center_y;
            row_count++;
            chars_per_row[row_count - 1] = 1;
        } else {
            if (prev_center_x != -1000 && (center_x - prev_center_x) > gap_threshold) {
                row_count++;
                chars_per_row[row_count - 1] = 1;
            } else {
                chars_per_row[row_count - 1]++;
            }
        }
        prev_center_x = center_x;
    }
    
    // Save character crops
    int char_count = 0;
    for (int i = 0; i < count; i++) {
        int w = data->boxes[i].maxx - data->boxes[i].minx + 1;
        int h = data->boxes[i].maxy - data->boxes[i].miny + 1;
        
        Image* crop = create_sub_image(data->work_img, data->boxes[i].minx, 
                                      data->boxes[i].miny, w, h);
        char fname[128];
        sprintf(fname, "../../outputs/list_detection/char_%03d.png", char_count++);
        save_image(fname, crop);
        free_image(crop);
    }
    
    // Organize into word folders and draw word boxes
    int current_char = 0;
    Image* debug_words = create_image(img->width, img->height, data->work_img->channels);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < data->work_img->channels; c++) {
                set_cached_pixel(debug_words, x, y, c, 
                               get_cached_pixel(data->work_img, x, y, c));
            }
        }
    }
    
    int char_index = 0;
    for (int w = 0; w < row_count; w++) {
        char foldername[128];
        sprintf(foldername, "../../outputs/list_detection/word_%d", w);
        MKDIR(foldername, 0755);
        
        for (int c = 0; c < chars_per_row[w]; c++) {
            if (current_char < char_count) {
                char oldname[128], newname[256];
                sprintf(oldname, "../../outputs/list_detection/char_%03d.png", current_char);
                sprintf(newname, "%s/char_%03d.png", foldername, c);
                rename(oldname, newname);
                current_char++;
            }
        }
        
        // Calculate word bounding box
        if (chars_per_row[w] <= 0) continue;
        
        int minx = 999999, miny = 999999;
        int maxx = -1, maxy = -1;
        
        for (int c = 0; c < chars_per_row[w]; c++) {
            if (char_index >= count) break;
            
            Box *b = &data->boxes[char_index];
            if (b->minx < minx) minx = b->minx;
            if (b->maxx > maxx) maxx = b->maxx;
            if (b->miny < miny) miny = b->miny;
            if (b->maxy > maxy) maxy = b->maxy;
            char_index++;
        }
        
        int word_w = maxx - minx + 1;
        int word_h = maxy - miny + 1;
        
        // Filter words
        int rxl = 0;
        if (Lan) {
            if (word_h > 80 || word_w > 200 || word_h <= 13)
                rxl = 1;
        }
        
        if (rxl == 1) {
            char cmd[256];
            sprintf(cmd, "rm -rf \"%s\"", foldername);
            system(cmd);
            continue;
        }
        
        draw_rect_green(debug_words, minx, miny, maxx, maxy);
    }
    
    save_image("../../outputs/list_detection/debug_words.png", debug_words);
    free_image(debug_words);
    
    // Save word stats
    FILE *out = fopen("../../outputs/list_detection/word_stats.txt", "w");
    if (out) {
        fprintf(out, "%d\n", row_count);
        for (int i = 0; i < row_count; i++) {
            fprintf(out, "%d ", chars_per_row[i]);
        }
        fprintf(out, "\n");
        fclose(out);
    }
    
    // Cleanup
    free(chars_per_row);
    
    return 0;
}

// Main detect function - creates shared preprocessing once
int detect(int argc, char **argv, Grid **grid) {
    if (!init_gtk(&argc, &argv)) {
        fprintf(stderr, "Failed to initialize GTK\n");
        return 1;
    }
    
    if (argc < 2) {
        printf("Usage: %s <image>\n", argv[0]);
        return 1;
    }
    
    Image* img = load_image(argv[1]);
    if (!img) {
        fprintf(stderr, "Error loading image: %s\n", argv[1]);
        return 1;
    }
    
    printf("Image loaded: %dx%d\n", img->width, img->height);
    
    // Preprocess image once - both functions will use this
    DetectionData* data = preprocess_image(img, 240); 
    if (!data) {
        fprintf(stderr, "Failed to preprocess image\n");
        free_image(img);
        return 1;
    }
    
    printf("Detecting grid...\n");
    *grid = detect_grid(img, data);
    
    if (!*grid) {
        fprintf(stderr, "Grid detection failed\n");
        free_detection_data(data, img->height);
        free_image(img);
        return 1;
    }
    
    printf("Detecting list...\n");
    int x1 = fmax((*grid)->bounds.top_left.x - 2,0);
    int y1 = (*grid)->bounds.top_left.y;
    int x2 = fmin((*grid)->bounds.bottom_right.x + 2,img->width);
    int y2 = (*grid)->bounds.bottom_right.y;
    
    int result = detect_list(img, data, x1, y1, x2, y2, argv[1]);
    
    if (result != 0) {
        fprintf(stderr, "List detection failed\n");
    } else {
        printf("List detection completed successfully\n");
    }
    
    free_detection_data(data, img->height);
    free_image(img);
    
    return result;
}