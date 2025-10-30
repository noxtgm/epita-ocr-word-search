#include "detect_list.h"

void clear_output_folder(const char *folder)
{
    DIR *dir = opendir(folder);
    if (!dir) {
        MKDIR(folder, 0755);
        printf("[INFO] Created folder: %s\n", folder);
        return;
    }

    struct dirent *entry;
    char path[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);
        unlink(path);
    }
    closedir(dir);
    printf("[INFO] Cleaned folder: %s\n", folder);
}

void flood_fill(unsigned char **binary, int **visited,
                int width, int height, int sx, int sy,
                int *minx, int *maxx, int *miny, int *maxy, int *pixel_count,
                int x1, int y1, int x2, int y2)
{
    Point *queue = malloc(width * height * sizeof(Point));
    int front = 0, rear = 0;

    queue[rear++] = (Point){sx, sy};
    visited[sy][sx] = 1;
    *pixel_count = 1;

    *minx = *maxx = sx;
    *miny = *maxy = sy;

    while (front < rear) {
        Point p = queue[front++];
        int x = p.x, y = p.y;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= width || ny >= height)
                    continue;

                // Ignore pixels inside crossword zone
                if (nx >= x1 && nx <= x2 && ny >= y1 && ny <= y2)
                    continue;

                if (visited[ny][nx]) continue;
                if (!binary[ny][nx]) continue;

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

// Simpler contrast enhancement that preserves thin letters
void enhance_contrast(GdkPixbuf *pixbuf) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    // Simple contrast stretch - make dark pixels darker and light pixels lighter
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            int gray = 0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2];
            
            // Enhanced contrast: darken below 150, lighten above
            if (gray < 150) {
                gray = gray * 0.7; // Darken dark areas
            } else {
                gray = 150 + (gray - 150) * 1.3; // Lighten light areas
            }
            
            gray = (gray < 0) ? 0 : (gray > 255) ? 255 : gray;
            p[0] = p[1] = p[2] = gray;
        }
    }
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    if (argc < 6) {
        printf("Usage: %s <image> x1 y1 x2 y2\n", argv[0]);
        return 1;
    }

    clear_output_folder("output");

    // Parse crossword coordinates
    int x1 = atoi(argv[2]);
    int y1 = atoi(argv[3]);
    int x2 = atoi(argv[4]);
    int y2 = atoi(argv[5]);

    printf("[INFO] Ignoring crossword zone: (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);

    // ---------- Load image ----------
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(argv[1], &error);
    if (!pixbuf) {
        fprintf(stderr, "Error loading image: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    int width  = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    printf("[INFO] Image loaded: %dx%d\n", width, height);

    // ---------- ENHANCE CONTRAST (preserves thin letters) ----------
    printf("[INFO] Enhancing contrast while preserving thin letters...\n");
    enhance_contrast(pixbuf);

    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    // ---------- Allocate binary + visited arrays ----------
    unsigned char **binary = malloc(height * sizeof(unsigned char*));
    int **visited = malloc(height * sizeof(int*));
    for (int y = 0; y < height; y++) {
        binary[y] = calloc(width, sizeof(unsigned char));
        visited[y] = calloc(width, sizeof(int));
    }

    // ---------- Convert to grayscale + LOWER THRESHOLD to detect thin letters ----------
    printf("[INFO] Using lower threshold to detect thin 'I' characters...\n");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            int gray = 0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2];
            binary[y][x] = (gray < 180) ? 1 : 0; // Lower threshold to catch thin letters
        }
    }

    // ---------- Detect connected components ----------
    Box *boxes = malloc(width * height * sizeof(Box));
    int box_count = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            // Skip pixels inside crossword zone
            if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
                continue;

            if (binary[y][x] && !visited[y][x]) {
                int minx, maxx, miny, maxy, pixel_count;
                flood_fill(binary, visited, width, height, x, y,
                           &minx, &maxx, &miny, &maxy, &pixel_count, x1, y1, x2, y2);

                int w = maxx - minx + 1;
                int h = maxy - miny + 1;

                // RELAXED FILTERING to preserve thin 'I' characters
                if (w < 3 || h < 5) continue;    // Allow thinner width for 'I'
                if (w > width / 2 || h > height / 2) continue;
                if (pixel_count < 15) continue;  // Lower pixel count for thin letters

                boxes[box_count] = (Box){minx, maxx, miny, maxy, pixel_count};
                box_count++;
            }
        }
    }

    printf("[INFO] Found %d character regions after filtering\n", box_count);

    // ---------- ORIGINAL SORTING LOGIC ----------
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

    qsort(boxes, box_count, sizeof(Box), cmp_boxes);

    // ---------- ORIGINAL GROUPING LOGIC ----------
    int *chars_per_row = malloc(box_count * sizeof(int));
    int row_count = 0;
    int current_row_y = -1000;
    int y_tol = 10;

    for (int i = 0; i < box_count; i++) {
        int center_y = (boxes[i].miny + boxes[i].maxy) / 2;

        if (abs(center_y - current_row_y) > y_tol) {
            current_row_y = center_y;
            row_count++;
            chars_per_row[row_count - 1] = 1;
        } else {
            chars_per_row[row_count - 1]++;
        }
    }

    printf("[INFO] Detected %d rows/words\n", row_count);

    // ---------- Save cropped regions ----------
    int char_count = 0;
    for (int i = 0; i < box_count; i++) {
        int minx = boxes[i].minx;
        int maxx = boxes[i].maxx;
        int miny = boxes[i].miny;
        int maxy = boxes[i].maxy;

        int w = maxx - minx + 1;
        int h = maxy - miny + 1;

        GdkPixbuf *crop = gdk_pixbuf_new_subpixbuf(pixbuf, minx, miny, w, h);
        char fname[64];
        sprintf(fname, "output/char_%03d.png", char_count++);
        gdk_pixbuf_save(crop, fname, "png", NULL, NULL);
        g_object_unref(crop);
    }

    printf("[INFO] Saved %d cropped characters.\n", char_count);

    // ---------- Organize characters into word folders ----------
    int current_char = 0;
    for (int w = 0; w < row_count; w++) {
        char foldername[64];
        sprintf(foldername, "output/word_%d", w);
        MKDIR(foldername, 0755);

        for (int c = 0; c < chars_per_row[w]; c++) {
            if (current_char < char_count) {
                char oldname[64], newname[128];
                sprintf(oldname, "output/char_%03d.png", current_char);
                sprintf(newname, "%s/char_%03d.png", foldername, c);
                rename(oldname, newname);
                current_char++;
            }
        }
        printf("[INFO] Word %d: %d characters\n", w, chars_per_row[w]);
    }
    printf("[INFO] Organized characters into %d folders.\n", row_count);

    // ---------- Write summary file ----------
    FILE *out = fopen("output/word_stats.txt", "w");
    if (out) {
        fprintf(out, "%d\n", row_count);
        for (int i = 0; i < row_count; i++) {
            fprintf(out, "%d ", chars_per_row[i]);
        }
        fprintf(out, "\n");
        fclose(out);
        printf("[INFO] Saved summary file: output/word_stats.txt\n");
    } else {
        printf("[ERROR] Could not write output/word_stats.txt\n");
    }

    printf("\n✅ Extracted %d characters across %d words.\n", char_count, row_count);
    printf("📁 Each word is stored in its own folder under 'output/'.\n");

    // ---------- Cleanup ----------
    for (int y = 0; y < height; y++) {
        free(binary[y]);
        free(visited[y]);
    }
    free(binary);
    free(visited);
    free(boxes);
    free(chars_per_row);
    g_object_unref(pixbuf);

    return 0;
}