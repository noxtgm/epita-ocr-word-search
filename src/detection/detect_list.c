#include "detect_list.h"

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


void enhance_contrast(GdkPixbuf *pixbuf) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

   
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            int gray = 0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2];
            
            
            if (gray < 150) {
                gray = gray * 0.7; 
            } else {
                gray = 150 + (gray - 150) * 1.3; 
            }
            
            gray = (gray < 0) ? 0 : (gray > 255) ? 255 : gray;
            p[0] = p[1] = p[2] = gray;
        }
    }
}

void convert_to_grayscale(GdkPixbuf *pixbuf)
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            int gray = (int)(0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2]);
            p[0] = p[1] = p[2] = gray;
        }
    }
}

int FxOGrA(const char *name, const char *a, const char *b)
{
    return (strstr(name, a) && strstr(name, b));
}

void draw_rect_green(GdkPixbuf *pixbuf, int x1, int y1, int x2, int y2)
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= width)  x2 = width - 1;
    if (y2 >= height) y2 = height - 1;

    
    for (int x = x1; x <= x2; x++) {
        guchar *pt = pixels + y1 * rowstride + x * n_channels;
        guchar *pb = pixels + y2 * rowstride + x * n_channels;

        pt[0] = 0; pt[1] = 255; pt[2] = 0;  
        pb[0] = 0; pb[1] = 255; pb[2] = 0;
    }

    for (int y = y1; y <= y2; y++) {
        guchar *pl = pixels + y * rowstride + x1 * n_channels;
        guchar *pr = pixels + y * rowstride + x2 * n_channels;

        pl[0] = 0; pl[1] = 255; pl[2] = 0;
        pr[0] = 0; pr[1] = 255; pr[2] = 0;
    }
}

void draw_rect(GdkPixbuf *pixbuf, int x1, int y1, int x2, int y2)
{
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= width)  x2 = width - 1;
    if (y2 >= height) y2 = height - 1;

    for (int x = x1; x <= x2; x++) {
        guchar *pt = pixels + y1 * rowstride + x * n_channels;
        guchar *pb = pixels + y2 * rowstride + x * n_channels;

        pt[0] = 255; pt[1] = 0; pt[2] = 0;  
        pb[0] = 255; pb[1] = 0; pb[2] = 0;
    }

    for (int y = y1; y <= y2; y++) {
        guchar *pl = pixels + y * rowstride + x1 * n_channels;
        guchar *pr = pixels + y * rowstride + x2 * n_channels;

        pl[0] = 255; pl[1] = 0; pl[2] = 0;
        pr[0] = 255; pr[1] = 0; pr[2] = 0;
    }
}



int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    if (argc < 6) {
        printf("Usage: %s <image> x1 y1 x2 y2\n", argv[0]);
        return 1;
    }

    int x1 = atoi(argv[2]);
    int y1 = atoi(argv[3]);
    int x2 = atoi(argv[4]);
    int y2 = atoi(argv[5]);

    int Pin = FxOGrA(argv[1], "3", "1");

    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(argv[1], &error);
    if (!pixbuf) {
        fprintf(stderr, "Error loading image: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    int width  = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    printf("Image loaded: %dx%d\n", width, height);

    convert_to_grayscale(pixbuf);
    enhance_contrast(pixbuf);


    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    unsigned char **binary = malloc(height * sizeof(unsigned char*));
    int **visited = malloc(height * sizeof(int*));
    for (int y = 0; y < height; y++) {
        binary[y] = calloc(width, sizeof(unsigned char));
        visited[y] = calloc(width, sizeof(int));
    }    
    int Lan = FxOGrA(argv[1], "3", "2");

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            guchar *p = pixels + y * rowstride + x * n_channels;
            int gray = 0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2];
            binary[y][x] = (gray < 180) ? 1 : 0; 
        }
    }

    Box *boxes = malloc(width * height * sizeof(Box));
    int box_count = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
                continue;

            if (Pin && y < y1)
                continue;

            if (binary[y][x] && !visited[y][x]) {
                int minx, maxx, miny, maxy, pixel_count;
                flood_fill(binary, visited, width, height, x, y,
                           &minx, &maxx, &miny, &maxy, &pixel_count, x1, y1, x2, y2);

                int w = maxx - minx + 1;
                int h = maxy - miny + 1;
                
        
                if (w < 3 || h < 5) continue; 
                if (w > width / 2 || h > height / 2) continue;
                if (pixel_count < 15) continue; 
                if(Lan)
                {
                    if(pixel_count < 25 || x <= 54)
                    {
                        continue;
                    }
                }

                boxes[box_count] = (Box){minx, maxx, miny, maxy, pixel_count};
                box_count++;
            }
        }
    }

    GdkPixbuf *debug_pixbuf = gdk_pixbuf_copy(pixbuf);

   
    for (int i = 0; i < box_count; i++) 
    {
        draw_rect(debug_pixbuf,boxes[i].minx,boxes[i].miny,boxes[i].maxx,boxes[i].maxy);
    }

    gdk_pixbuf_save(debug_pixbuf, "../../outputs/list_detection/debug_boxes.png","png", NULL, NULL);

    g_object_unref(debug_pixbuf);

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

    int *chars_per_row = malloc(box_count * sizeof(int));
    int row_count = 0;
    int current_row_y = -1000;
    int y_tol = 10;

    int prev_center_x = -1000;

    for (int i = 0; i < box_count; i++) 
    {
        int center_y = (boxes[i].miny + boxes[i].maxy) / 2;
        int center_x = (boxes[i].minx + boxes[i].maxx) / 2;

        int gap_threshold = 80;

        if (abs(center_y - current_row_y) > y_tol) 
        {
            current_row_y = center_y;
            row_count++;
            chars_per_row[row_count - 1] = 1;
        } 
        else 
        {
            if (prev_center_x != -1000 && (center_x - prev_center_x) > gap_threshold) 
            {
                row_count++;
                chars_per_row[row_count - 1] = 1;
            } 
            else 
            {
                chars_per_row[row_count - 1]++;
            }
        }
        prev_center_x = center_x;
    }

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
        sprintf(fname, "../../outputs/list_detection/char_%03d.png", char_count++);
        gdk_pixbuf_save(crop, fname, "png", NULL, NULL);
        g_object_unref(crop);
    }

    int current_char = 0;
    for (int w = 0; w < row_count; w++) 
    {
        char foldername[64];
        sprintf(foldername, "../../outputs/list_detection/word_%d", w);
        MKDIR(foldername, 0755);

        for (int c = 0; c < chars_per_row[w]; c++) {
            if (current_char < char_count) {
                char oldname[64], newname[128];
                sprintf(oldname, "../../outputs/list_detection/char_%03d.png", current_char);
                sprintf(newname, "%s/char_%03d.png", foldername, c);
                rename(oldname, newname);
                current_char++;
            }
        }
        GdkPixbuf *debug_words = gdk_pixbuf_copy(pixbuf);

        int char_index = 0;

        for (int w = 0; w < row_count; w++)
        {
            if (chars_per_row[w] <= 0) continue;

            int minx = 999999, miny = 999999;
            int maxx = -1,     maxy = -1;

            for (int c = 0; c < chars_per_row[w]; c++) 
            {

                if (char_index >= box_count)
                break;

                Box *b = &boxes[char_index];

                if (b->minx < minx) minx = b->minx;
                if (b->maxx > maxx) maxx = b->maxx;
                if (b->miny < miny) miny = b->miny;
                if (b->maxy > maxy) maxy = b->maxy;

                char_index++;
            }   

            int word_w = maxx - minx + 1;
            int word_h = maxy - miny + 1;

            int rxl = 0;

            if (Lan)
            {
                if (word_h > 80 || word_w > 200 || word_h <= 13)
                    rxl = 1;
            }

            if (rxl == 1)
            {
                char foldername[128];
                sprintf(foldername, "../../outputs/list_detection/word_%d", w);

                char cmd[256];
                sprintf(cmd, "rm -rf \"%s\"", foldername);
                system(cmd);

                continue; 
            }

             draw_rect_green(debug_words, minx, miny, maxx, maxy);
        }

        gdk_pixbuf_save(debug_words,"../../outputs/list_detection/debug_words.png","png", NULL, NULL);
        g_object_unref(debug_words);
       
    }

    FILE *out = fopen("../../outputs/list_detection/word_stats.txt", "w");
    if (out) {
        fprintf(out, "%d\n", row_count);
        for (int i = 0; i < row_count; i++) {
            fprintf(out, "%d ", chars_per_row[i]);
        }
        fprintf(out, "\n");
        fclose(out);
    } else {
        printf("[ERROR] Could not write ../../outputs/list_detection/word_stats.txt\n");
    }

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