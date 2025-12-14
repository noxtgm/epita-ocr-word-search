#define _POSIX_C_SOURCE 200809L
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define NUM_STEPS 5

typedef enum {
    STEP_IMPORT = 0,
    STEP_ROTATION,
    STEP_DETECTION,
    STEP_OCR,
    STEP_SOLVE
} StepType;

typedef struct {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *load_button;
    GtkWidget *title_label;
    GtkWidget *image_display;
    GtkWidget *image_container;
    GtkWidget *run_button;
    GtkWidget *run_all_button;
    GtkWidget *console_view;
    GtkTextBuffer *console_buffer;
    GtkWidget *step_buttons[NUM_STEPS];
    
    char *input_image_path;
    char *current_image_path;  // Current image being displayed (original or rotated)
    StepType current_step;
    gboolean steps_completed[NUM_STEPS];
    
    GdkPixbuf *current_pixbuf;
    GdkPixbuf *original_pixbuf;
} AppData;

static const char *step_names[] = {
    "1. Import",
    "2. Rotation",
    "3. Detection",
    "4. OCR",
    "5. Solver"
};

typedef struct {
    char word[100];
    int start_col, start_row;
    int end_col, end_row;
} WordSolution;

typedef struct {
    int row, col;
    int x1, y1, x2, y2;
} CellPosition;

// Parse solver output to get word coordinates
static gboolean parse_solver_output(const char *output, int *start_col, int *start_row, 
                                      int *end_col, int *end_row) {
    // Format: (col1,row1)(col2,row2)
    if (sscanf(output, "(%d,%d)(%d,%d)", start_col, start_row, end_col, end_row) == 4) {
        return TRUE;
    }
    return FALSE;
}

// Load cell positions from detection output
static CellPosition* load_cell_positions(int *num_cells, int *grid_rows, int *grid_cols) {
    FILE *fp = fopen("../outputs/grid_detection/cell_positions.txt", "r");
    if (!fp) return NULL;
    
    if (fscanf(fp, "%d %d", grid_rows, grid_cols) != 2) {
        fclose(fp);
        return NULL;
    }
    
    *num_cells = (*grid_rows) * (*grid_cols);
    CellPosition *cells = malloc(*num_cells * sizeof(CellPosition));
    if (!cells) {
        fclose(fp);
        return NULL;
    }
    
    int idx = 0;
    while (idx < *num_cells && 
           fscanf(fp, "%d %d %d %d %d %d", 
                  &cells[idx].row, &cells[idx].col,
                  &cells[idx].x1, &cells[idx].y1,
                  &cells[idx].x2, &cells[idx].y2) == 6) {
        idx++;
    }
    
    fclose(fp);
    
    if (idx != *num_cells) {
        free(cells);
        return NULL;
    }
    
    return cells;
}

// Get cell center coordinates
static gboolean get_cell_center(CellPosition *cells, int num_cells, int row, int col,
                                 int *center_x, int *center_y) {
    for (int i = 0; i < num_cells; i++) {
        if (cells[i].row == row && cells[i].col == col) {
            *center_x = (cells[i].x1 + cells[i].x2) / 2;
            *center_y = (cells[i].y1 + cells[i].y2) / 2;
            return TRUE;
        }
    }
    return FALSE;
}

// Draw a line on the image
static void draw_line_on_image(GdkPixbuf *pixbuf, int x1, int y1, int x2, int y2,
                               guchar r, guchar g, guchar b, int thickness) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1;
    int y = y1;
    
    while (1) {
        // Draw thick line by drawing surrounding pixels
        for (int ty = -thickness; ty <= thickness; ty++) {
            for (int tx = -thickness; tx <= thickness; tx++) {
                int px = x + tx;
                int py = y + ty;
                
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    guchar *pixel = pixels + py * rowstride + px * channels;
                    pixel[0] = r;
                    pixel[1] = g;
                    pixel[2] = b;
                    if (channels == 4) pixel[3] = 255;
                }
            }
        }
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

// Create annotated image with found words
static gboolean create_solved_image(const char *input_image, const char *output_image,
                                     WordSolution *solutions, int num_solutions) {
    // Load cell positions
    int num_cells, grid_rows, grid_cols;
    CellPosition *cells = load_cell_positions(&num_cells, &grid_rows, &grid_cols);
    if (!cells) {
        return FALSE;
    }
    
    // Load the input image
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(input_image, &error);
    if (error) {
        g_error_free(error);
        free(cells);
        return FALSE;
    }
    
    // Make a copy to draw on
    GdkPixbuf *annotated = gdk_pixbuf_copy(pixbuf);
    g_object_unref(pixbuf);
    
    if (!annotated) {
        free(cells);
        return FALSE;
    }
    
    // Draw lines for each found word
    // Use different colors for variety
    guchar colors[][3] = {
        {255, 0, 0},     // Red
        {0, 255, 0},     // Green
        {0, 0, 255},     // Blue
        {255, 255, 0},   // Yellow
        {255, 0, 255},   // Magenta
        {0, 255, 255},   // Cyan
        {255, 128, 0},   // Orange
        {128, 0, 255}    // Purple
    };
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    
    for (int i = 0; i < num_solutions; i++) {
        int x1, y1, x2, y2;
        if (get_cell_center(cells, num_cells, solutions[i].start_row, solutions[i].start_col, &x1, &y1) &&
            get_cell_center(cells, num_cells, solutions[i].end_row, solutions[i].end_col, &x2, &y2)) {
            
            guchar *color = colors[i % num_colors];
            draw_line_on_image(annotated, x1, y1, x2, y2, color[0], color[1], color[2], 2);
        }
    }
    
    gboolean success = gdk_pixbuf_save(annotated, output_image, "png", &error, NULL);
    if (error) {
        g_error_free(error);
        success = FALSE;
    }
    
    g_object_unref(annotated);
    free(cells);
    
    return success;
}

// Function prototypes
static void load_image_clicked(GtkWidget *widget, gpointer data);
static void step_clicked(GtkWidget *widget, gpointer data);
static void run_step_clicked(GtkWidget *widget, gpointer data);
static void run_all_steps_clicked(GtkWidget *widget, gpointer data);
static void display_image(AppData *app, const char *image_path);
static void on_image_container_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer data);
static void update_step_buttons(AppData *app);
static void create_main_ui(AppData *app);
static void log_message(AppData *app, const char *message);
static void reset_workflow(AppData *app);
static void initialize_output_directories(void);
static void clean_output_directories(void);

// Check if file exists
static gboolean file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

// Initialize all output directories
static void initialize_output_directories(void) {
    // Create base outputs directory
    mkdir("../outputs", 0755);
    
    // Create subdirectories for each module
    mkdir("../outputs/rotation", 0755);
    mkdir("../outputs/grid_detection", 0755);
    mkdir("../outputs/grid_detection/cells", 0755);
    mkdir("../outputs/list_detection", 0755);
    mkdir("../outputs/recognized_files", 0755);
    mkdir("../outputs/solved", 0755);
}

// Clean all output directories
static void clean_output_directories(void) {
    system("rm -rf ../outputs/rotation/* 2>/dev/null");
    system("rm -rf ../outputs/grid_detection/* 2>/dev/null");
    system("rm -rf ../outputs/list_detection/* 2>/dev/null");
    system("rm -rf ../outputs/recognized_files/* 2>/dev/null");
    system("rm -rf ../outputs/solved/* 2>/dev/null");
    mkdir("../outputs/grid_detection/cells", 0755);
}

// Log message to console
static void log_message(AppData *app, const char *message) {
    if (!app->console_buffer) return;
    
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app->console_buffer, &iter);
    
    gtk_text_buffer_insert(app->console_buffer, &iter, message, -1);
    gtk_text_buffer_insert(app->console_buffer, &iter, "\n", -1);
    
    // Auto-scroll to bottom
    GtkTextMark *mark = gtk_text_buffer_get_insert(app->console_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->console_view), mark, 0.0, TRUE, 0.0, 1.0);
    
    // Force update
    while (gtk_events_pending()) gtk_main_iteration();
}

// Handle image container resize
static void on_image_container_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer data) {
    (void)widget;
    AppData *app = (AppData *)data;
    
    if (!app->original_pixbuf) return;
    
    // Get original dimensions
    int orig_width = gdk_pixbuf_get_width(app->original_pixbuf);
    int orig_height = gdk_pixbuf_get_height(app->original_pixbuf);
    
    // Calculate available space with 5px padding on each side
    int max_width = allocation->width - 10;
    int max_height = allocation->height - 10;
    
    // Ensure minimum size
    if (max_width < 50 || max_height < 50) return;
    
    // Calculate scale to fit while maintaining aspect ratio
    double scale_w = (double)max_width / orig_width;
    double scale_h = (double)max_height / orig_height;
    double scale = (scale_w < scale_h) ? scale_w : scale_h;
    
    int new_width = (int)(orig_width * scale);
    int new_height = (int)(orig_height * scale);
    
    if (app->current_pixbuf) {
        int current_width = gdk_pixbuf_get_width(app->current_pixbuf);
        int current_height = gdk_pixbuf_get_height(app->current_pixbuf);
        
        if (abs(current_width - new_width) < 5 && abs(current_height - new_height) < 5) {
            return;
        }
    }
    
    // Create scaled pixbuf
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(app->original_pixbuf, new_width, new_height, GDK_INTERP_BILINEAR);
    
    if (app->current_pixbuf) {
        g_object_unref(app->current_pixbuf);
    }
    app->current_pixbuf = scaled;
    
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_display), scaled);
}

// Display image in the main area
static void display_image(AppData *app, const char *image_path) {
    if (!file_exists(image_path)) {
        log_message(app, "Error: Image file not found");
        return;
    }
    
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(image_path, &error);
    
    if (error) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Error loading image: %s", error->message);
        log_message(app, msg);
        g_error_free(error);
        return;
    }
    
    // Store original pixbuf
    if (app->original_pixbuf) {
        g_object_unref(app->original_pixbuf);
    }
    app->original_pixbuf = pixbuf;
    
    if (app->current_image_path) {
        g_free(app->current_image_path);
    }
    app->current_image_path = g_strdup(image_path);
    
    if (app->image_container) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(app->image_container, &allocation);
        on_image_container_size_allocate(app->image_container, &allocation, app);
    } else {
        if (app->current_pixbuf) {
            g_object_unref(app->current_pixbuf);
        }
        app->current_pixbuf = g_object_ref(pixbuf);
        gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_display), pixbuf);
    }
}

// Reset workflow and clean modules
static void reset_workflow(AppData *app) {
    
    // Clear all step completions except import
    for (int i = 1; i < NUM_STEPS; i++) {
        app->steps_completed[i] = FALSE;
    }
    
    // Clean all module outputs
    system("cd detection && make clean 2>&1");
    system("cd neural_network && make clean 2>&1");
    system("cd rotation && make clean 2>&1");
    system("cd solver && make clean 2>&1");
}

// Update step button states
static void update_step_buttons(AppData *app) {
    for (int i = 0; i < NUM_STEPS; i++) {
        GtkWidget *button = app->step_buttons[i];
        GtkStyleContext *context = gtk_widget_get_style_context(button);
        
        // Remove all style classes
        gtk_style_context_remove_class(context, "suggested-action");
        
        // Check if step is accessible
        gboolean accessible = TRUE;
        if (i > 0 && !app->steps_completed[i - 1]) {
            accessible = FALSE;
        }
        
        gtk_widget_set_sensitive(button, accessible);
        
        // Highlight current step
        if (i == (int)app->current_step) {
            gtk_style_context_add_class(context, "suggested-action");
        }
    }
    
    // Update run button state and text
    gboolean run_button_enabled = TRUE;
    
    // Check if current step is accessible
    if (app->current_step > 0 && !app->steps_completed[app->current_step - 1]) {
        run_button_enabled = FALSE;
    }
    
    gtk_widget_set_sensitive(app->run_button, run_button_enabled);
    
    // Update run button text based on current step
    if (app->current_step == STEP_IMPORT && app->steps_completed[STEP_IMPORT]) {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Load new image");
    } else if (app->steps_completed[app->current_step]) {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Re-run step");
    } else {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Run step");
    }
}

// Run the current step
static void run_step_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData *)data;
    
    char command[8192];
    char output_path[2048];
    FILE *fp;
    
    switch (app->current_step) {
        case STEP_IMPORT:
            if (!app->input_image_path) {
                log_message(app, "Please load an image first");
                return;
            }
            
            if (app->steps_completed[STEP_IMPORT]) {
                log_message(app, "Loading new image...");
                load_image_clicked(NULL, app);
                return;
            }
            
            app->steps_completed[STEP_IMPORT] = TRUE;
            display_image(app, app->input_image_path);
            log_message(app, "Image imported successfully");
            break;
            
        case STEP_ROTATION:
            log_message(app, "");
            
            // Build rotation_detector
            system("cd rotation && make 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Run rotation_detector and capture output
            snprintf(command, sizeof(command), 
                     "cd rotation && ./rotation_detector \"%s\" 2>&1", 
                     app->input_image_path);
            
            fp = popen(command, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    if (strlen(line) > 0) {
                        log_message(app, line);
                    }
                }
                pclose(fp);
            }
            
            // Check if rotated image was created
            const char *basename = strrchr(app->input_image_path, '/');
            basename = basename ? basename + 1 : app->input_image_path;
            
            snprintf(output_path, sizeof(output_path), 
                     "../outputs/rotation/%s", basename);
            
            if (file_exists(output_path)) {
                app->steps_completed[STEP_ROTATION] = TRUE;
                display_image(app, output_path);
                log_message(app, "Displaying rotated image...");
                log_message(app, "Image rotated successfully");
            } else {
                app->steps_completed[STEP_ROTATION] = TRUE;
            }
            break;
            
        case STEP_DETECTION:
            log_message(app, "");  // Empty line before detection step
            log_message(app, "Detecting grid and word list...");
            
            // Ensure cells folder exists before detection
            mkdir("../outputs/grid_detection/cells", 0755);
            
            // Check if rotated image exists
            const char *basename_det = strrchr(app->input_image_path, '/');
            basename_det = basename_det ? basename_det + 1 : app->input_image_path;
            
            char rotated_path[2048];
            snprintf(rotated_path, sizeof(rotated_path), "../outputs/rotation/%s", basename_det);
            
            const char *detect_image;
            if (file_exists(rotated_path)) {
                detect_image = rotated_path;
            } else {
                detect_image = app->input_image_path;
            }
            
            char absolute_path[4096];
            if (detect_image[0] != '/') {
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    snprintf(absolute_path, sizeof(absolute_path), "%s/%s", cwd, detect_image);
                    detect_image = absolute_path;
                }
            }
            
            // Build detection
            system("cd detection && make 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Run detection
            snprintf(command, sizeof(command), 
                     "cd detection && ./detect \"%s\" 2>&1", 
                     detect_image);
            
            fp = popen(command, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    if (strlen(line) > 0) {
                        log_message(app, line);
                    }
                }
                pclose(fp);
            }
            
            // Check if debug image exists
            if (file_exists("../outputs/grid_detection/debug.png")) {
                app->steps_completed[STEP_DETECTION] = TRUE;
                
                // Check if both debug images exist to combine them
                if (file_exists("../outputs/list_detection/debug_boxes.png")) {
                    // Load both images
                    GError *error = NULL;
                    GdkPixbuf *grid_img = gdk_pixbuf_new_from_file("../outputs/grid_detection/debug.png", &error);
                    if (error) { g_error_free(error); error = NULL; }
                    
                    GdkPixbuf *list_img = gdk_pixbuf_new_from_file("../outputs/list_detection/debug_words.png", &error);
                    if (error) { g_error_free(error); error = NULL; }
                    
                    if (grid_img && list_img) {
                        // Get dimensions
                        int grid_w = gdk_pixbuf_get_width(grid_img);
                        int grid_h = gdk_pixbuf_get_height(grid_img);
                        int list_w = gdk_pixbuf_get_width(list_img);
                        int list_h = gdk_pixbuf_get_height(list_img);
                        
                        // Create combined image
                        int total_w = grid_w + list_w;
                        int total_h = (grid_h > list_h) ? grid_h : list_h;
                        GdkPixbuf *combined = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, total_w, total_h);
                        gdk_pixbuf_fill(combined, 0xFFFFFFFF);
                        
                        // Copy both images side by side
                        gdk_pixbuf_copy_area(grid_img, 0, 0, grid_w, grid_h, combined, 0, 0);
                        gdk_pixbuf_copy_area(list_img, 0, 0, list_w, list_h, combined, grid_w, 0);
                        
                        // Save and display combined image
                        gdk_pixbuf_save(combined, "../outputs/grid_detection/combined_detection.png", "png", &error, NULL);
                        if (!error) {
                            display_image(app, "../outputs/grid_detection/combined_detection.png");
                        } else {
                            g_error_free(error);
                            display_image(app, "../outputs/grid_detection/debug.png");
                        }
                        
                        g_object_unref(grid_img);
                        g_object_unref(list_img);
                        g_object_unref(combined);
                    } else {
                        if (grid_img) g_object_unref(grid_img);
                        if (list_img) g_object_unref(list_img);
                        display_image(app, "../outputs/grid_detection/debug.png");
                    }
                } else {
                display_image(app, "../outputs/grid_detection/debug.png");
                }
                
                log_message(app, "Displaying detected letters...");
            } else {
                log_message(app, "Error: Detection failed - debug image not found");
            }
            log_message(app, "Detection completed successfully");
            break;
            
        case STEP_OCR:
            log_message(app, "");
            log_message(app, "Identifying characters...");
            
            // Build OCR
            system("cd neural_network && make recognize 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Run OCR recognition
            snprintf(command, sizeof(command), 
                     "cd neural_network && ./file_creator 2>&1");
            
            fp = popen(command, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    line[strcspn(line, "\n")] = 0;
                    if (strlen(line) > 0 && strncmp(line, "make", 4) != 0) {
                        log_message(app, line);
                    }
                }
                pclose(fp);
            }
            
            // Check if output files exist
            if (file_exists("../outputs/recognized_files/recognized_grid.txt") &&
                file_exists("../outputs/recognized_files/recognized_words.txt")) {
                app->steps_completed[STEP_OCR] = TRUE;
                
                // Display the image being processed
                const char *basename_ocr = strrchr(app->input_image_path, '/');
                basename_ocr = basename_ocr ? basename_ocr + 1 : app->input_image_path;
                
                char rotated_ocr_path[2048];
                snprintf(rotated_ocr_path, sizeof(rotated_ocr_path), "../outputs/rotation/%s", basename_ocr);
                
                if (file_exists(rotated_ocr_path)) {
                    display_image(app, rotated_ocr_path);
                } else {
                    display_image(app, app->input_image_path);
                }
                
                // Display recognized grid
                log_message(app, "=== Recognized Grid ===");
                FILE *grid_file = fopen("../outputs/recognized_files/recognized_grid.txt", "r");
                if (grid_file) {
                    char line[256];
                    int line_count = 0;
                    while (fgets(line, sizeof(line), grid_file) && line_count < 5) {
                        line[strcspn(line, "\n")] = 0;
                        log_message(app, line);
                        line_count++;
                    }
                    if (line_count == 5) log_message(app, "...");
                    fclose(grid_file);
                }
                
                // Display recognized words
                log_message(app, "=== Recognized Words ===");
                FILE *words_file = fopen("../outputs/recognized_files/recognized_words.txt", "r");
                if (words_file) {
                    char line[256];
                    int word_count = 0;
                    while (fgets(line, sizeof(line), words_file) && word_count < 10) {
                        line[strcspn(line, "\n")] = 0;
                        if (strlen(line) > 0) {
                            log_message(app, line);
                            word_count++;
                        }
                    }
                    if (word_count == 10) log_message(app, "...");
                    fclose(words_file);
                }
                
                log_message(app, "OCR completed successfully");
            } else {
                log_message(app, "Error: OCR failed - output files not found");
            }
            break;
            
        case STEP_SOLVE:
            log_message(app, "");
            log_message(app, "Solving word search...");
            system("cd solver && make 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Read word list and solve for each word
            FILE *words_file = fopen("../outputs/recognized_files/recognized_words.txt", "r");
            if (!words_file) {
                log_message(app, "Error: Could not open word list file");
                break;
            }
            
            // Array to store found word solutions
            WordSolution *solutions = malloc(100 * sizeof(WordSolution));
            int num_solutions = 0;
            
            char word[100];
            int found_count = 0;
            int total_count = 0;
            
            while (fgets(word, sizeof(word), words_file)) {
                word[strcspn(word, "\n")] = 0;
                if (strlen(word) == 0) continue;
                
                total_count++;
                
                snprintf(command, sizeof(command),
                         "cd solver && ./solver \"../../outputs/recognized_files/recognized_grid.txt\" \"%s\" 2>&1",
                         word);
                
                fp = popen(command, "r");
                if (fp) {
                    char result_line[256];
                    if (fgets(result_line, sizeof(result_line), fp)) {
                        result_line[strcspn(result_line, "\n")] = 0;
                        
                        if (strstr(result_line, "Not found") == NULL) {
                            char msg[512];
                            snprintf(msg, sizeof(msg), "✓ %s: %s", word, result_line);
                            log_message(app, msg);
                            found_count++;
                            
                            if (num_solutions < 100) {
                                strncpy(solutions[num_solutions].word, word, 99);
                                solutions[num_solutions].word[99] = '\0';
                                parse_solver_output(result_line, 
                                                   &solutions[num_solutions].start_col,
                                                   &solutions[num_solutions].start_row,
                                                   &solutions[num_solutions].end_col,
                                                   &solutions[num_solutions].end_row);
                                num_solutions++;
                            }
                        } else {
                            char msg[512];
                            snprintf(msg, sizeof(msg), "✗ %s: Not found", word);
                            log_message(app, msg);
                        }
                    }
                    pclose(fp);
                }
            }
            fclose(words_file);
            
            char summary[256];
            snprintf(summary, sizeof(summary), "%d/%d words found", found_count, total_count);
            log_message(app, summary);
            
            log_message(app, "Creating annotated image...");
            
            const char *base_basename = strrchr(app->input_image_path, '/');
            base_basename = base_basename ? base_basename + 1 : app->input_image_path;
            
            char base_image_path[2048];
            snprintf(base_image_path, sizeof(base_image_path), 
                     "../outputs/rotation/%s", base_basename);
            
            const char *input_for_annotation;
            if (file_exists(base_image_path)) {
                input_for_annotation = base_image_path;
            } else {
                input_for_annotation = app->input_image_path;
            }
            
            char solved_image_path[2048];
            snprintf(solved_image_path, sizeof(solved_image_path), 
                     "../outputs/solved/%s", base_basename);
            
            if (create_solved_image(input_for_annotation, solved_image_path,
                                   solutions, num_solutions)) {
                if (app->original_pixbuf) {
                    g_object_unref(app->original_pixbuf);
                    app->original_pixbuf = NULL;
                }
                if (app->current_pixbuf) {
                    g_object_unref(app->current_pixbuf);
                    app->current_pixbuf = NULL;
                }
                    
                display_image(app, solved_image_path);
                
                gtk_widget_queue_draw(app->image_display);
    
                while (gtk_events_pending()) {
                    gtk_main_iteration();
                }
            } else {
                log_message(app, "Warning: Failed to create annotated image");
            }
            
            free(solutions);
            app->steps_completed[STEP_SOLVE] = TRUE;
            log_message(app, "Word search solved successfully!");
            break;
    }
    
    update_step_buttons(app);
}

// Run all steps from start to finish
static void run_all_steps_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData *)data;
    
    if (!app->input_image_path) {
        log_message(app, "Please load an image first");
        return;
    }
    
    // Step 1: Import
    if (!app->steps_completed[STEP_IMPORT]) {
        app->current_step = STEP_IMPORT;
        run_step_clicked(NULL, app);
    }
    
    // Step 2: Rotation
    if (app->steps_completed[STEP_IMPORT]) {
        app->current_step = STEP_ROTATION;
        run_step_clicked(NULL, app);
    }
    
    // Step 3: Detection
    if (app->steps_completed[STEP_ROTATION]) {
        app->current_step = STEP_DETECTION;
        run_step_clicked(NULL, app);
    }
    
    // Step 4: OCR
    if (app->steps_completed[STEP_DETECTION]) {
        app->current_step = STEP_OCR;
        run_step_clicked(NULL, app);
    }
    
    // Step 5: Solve
    if (app->steps_completed[STEP_OCR]) {
        app->current_step = STEP_SOLVE;
        run_step_clicked(NULL, app);
    }
}

// Handle step button clicks
static void step_clicked(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    
    // Find which step was clicked
    for (int i = 0; i < NUM_STEPS; i++) {
        if (app->step_buttons[i] == widget) {
            app->current_step = i;
            break;
        }
    }
    
    char output_path[512];
    const char *basename;
    
    switch (app->current_step) {
        case STEP_IMPORT:
            if (app->steps_completed[STEP_IMPORT] && app->input_image_path) {
                display_image(app, app->input_image_path);
            }
            break;
            
        case STEP_ROTATION:
            if (app->steps_completed[STEP_IMPORT]) {
                basename = strrchr(app->input_image_path, '/');
                basename = basename ? basename + 1 : app->input_image_path;
                
                snprintf(output_path, sizeof(output_path), 
                         "../outputs/rotation/%s", basename);
                
                if (file_exists(output_path)) {
                    display_image(app, output_path);
                } else {
                    display_image(app, app->input_image_path);
                }
            }
            break;
            
        case STEP_DETECTION:
            if (app->steps_completed[STEP_DETECTION]) {
                if (file_exists("../outputs/grid_detection/combined_detection.png")) {
                    display_image(app, "../outputs/grid_detection/combined_detection.png");
                } else if (file_exists("../outputs/grid_detection/debug.png")) {
                    display_image(app, "../outputs/grid_detection/debug.png");
                }
            }
            break;
            
        case STEP_OCR:
            if (app->steps_completed[STEP_IMPORT]) {
                basename = strrchr(app->input_image_path, '/');
                basename = basename ? basename + 1 : app->input_image_path;
                
                snprintf(output_path, sizeof(output_path), 
                         "../outputs/rotation/%s", basename);
                
                if (file_exists(output_path)) {
                    display_image(app, output_path);
                } else {
                    display_image(app, app->input_image_path);
                }
            }
            break;
            
        case STEP_SOLVE:
            if (app->steps_completed[STEP_SOLVE]) {
                // Check for solved image with same name as original in solved folder
                basename = strrchr(app->input_image_path, '/');
                basename = basename ? basename + 1 : app->input_image_path;
                
                char solved_path[2048];
                snprintf(solved_path, sizeof(solved_path), 
                         "../outputs/solved/%s", basename);
                
                if (file_exists(solved_path)) {
                    display_image(app, solved_path);
                } else {
                    snprintf(output_path, sizeof(output_path), 
                             "../outputs/rotation/%s", basename);
                    
                    if (file_exists(output_path)) {
                        display_image(app, output_path);
                    } else {
                        display_image(app, app->input_image_path);
                    }
                }
            } else if (app->steps_completed[STEP_IMPORT]) {
                basename = strrchr(app->input_image_path, '/');
                basename = basename ? basename + 1 : app->input_image_path;
                
                snprintf(output_path, sizeof(output_path), 
                         "../outputs/rotation/%s", basename);
                
                if (file_exists(output_path)) {
                    display_image(app, output_path);
                } else {
                    display_image(app, app->input_image_path);
                }
            }
            break;
            
        default:
            break;
    }
    
    update_step_buttons(app);
}

// Create the main UI after image is loaded
static void create_main_ui(AppData *app) {
    // Remove load button and title label
    gtk_widget_destroy(app->load_button);
    if (app->title_label) {
        gtk_widget_destroy(app->title_label);
        app->title_label = NULL;
    }
    
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(app->main_box), hbox, TRUE, TRUE, 0);
    
    // Left pane
    GtkWidget *left_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(left_pane, 200, -1);
    gtk_widget_set_name(left_pane, "side-panel");
    gtk_box_pack_start(GTK_BOX(hbox), left_pane, FALSE, FALSE, 0);
    
    // Step buttons
    for (int i = 0; i < NUM_STEPS; i++) {
        app->step_buttons[i] = gtk_button_new_with_label(step_names[i]);
        gtk_widget_set_size_request(app->step_buttons[i], -1, 40);
        gtk_box_pack_start(GTK_BOX(left_pane), app->step_buttons[i], FALSE, FALSE, 0);
        g_signal_connect(app->step_buttons[i], "clicked", G_CALLBACK(step_clicked), app);
    }
    
    // Run all steps button
    app->run_all_button = gtk_button_new_with_label("Run all steps");
    gtk_widget_set_size_request(app->run_all_button, -1, 50);
    gtk_box_pack_end(GTK_BOX(left_pane), app->run_all_button, FALSE, FALSE, 5);
    g_signal_connect(app->run_all_button, "clicked", G_CALLBACK(run_all_steps_clicked), app);
    
    // Run button
    app->run_button = gtk_button_new_with_label("Run step");
    gtk_widget_set_size_request(app->run_button, -1, 50);
    gtk_box_pack_end(GTK_BOX(left_pane), app->run_button, FALSE, FALSE, 5);
    g_signal_connect(app->run_button, "clicked", G_CALLBACK(run_step_clicked), app);
    
    // Main display area
    GtkWidget *right_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(right_pane, "main-view");
    gtk_box_pack_start(GTK_BOX(hbox), right_pane, TRUE, TRUE, 0);
    
    // Image display
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                   GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    gtk_widget_set_name(scrolled, "image-container");
    gtk_box_pack_start(GTK_BOX(right_pane), scrolled, TRUE, TRUE, 0);
    
    app->image_container = scrolled;
    app->image_display = gtk_image_new();
    gtk_widget_set_halign(app->image_display, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(app->image_display, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(scrolled), app->image_display);
    g_signal_connect(scrolled, "size-allocate", G_CALLBACK(on_image_container_size_allocate), app);
    
    // Console log area at bottom
    GtkWidget *console_frame = gtk_frame_new("Console Output");
    gtk_widget_set_name(console_frame, "console-frame");
    gtk_box_pack_end(GTK_BOX(app->main_box), console_frame, FALSE, FALSE, 0);
    
    GtkWidget *console_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(console_scroll),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(console_scroll, -1, 200);
    gtk_container_add(GTK_CONTAINER(console_frame), console_scroll);
    
    app->console_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->console_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app->console_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->console_view), GTK_WRAP_WORD);
    gtk_widget_set_name(app->console_view, "console-text");
    gtk_container_add(GTK_CONTAINER(console_scroll), app->console_view);
    
    app->console_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->console_view));
    
    // Initial message
    log_message(app, "Image successfully loaded");
    
    // Set initial step to IMPORT
    app->current_step = STEP_IMPORT;
    app->steps_completed[STEP_IMPORT] = TRUE;
    
    // Display the loaded image
    display_image(app, app->input_image_path);
    
    update_step_buttons(app);
    gtk_widget_show_all(app->window);
}

// Handle image load button click
static void load_image_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    AppData *app = (AppData *)data;
    
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Image",
                                                     GTK_WINDOW(app->window),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                                     "_Cancel", GTK_RESPONSE_CANCEL,
                                                     "_Open", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Image files");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (app->input_image_path) {
            g_free(app->input_image_path);
        }
        app->input_image_path = filename;
        
        // Clean output directories when loading new image
        clean_output_directories();
        
        // Clear console for new image
        if (app->console_buffer) {
            gtk_text_buffer_set_text(app->console_buffer, "", -1);
        }
        
        // If UI already exists, reset workflow
        if (app->console_buffer) {
            reset_workflow(app);
            app->steps_completed[STEP_IMPORT] = TRUE;
            display_image(app, app->input_image_path);
            log_message(app, "New image loaded successfully");
            app->current_step = STEP_IMPORT;
            update_step_buttons(app);
        } else {
            // Create main UI for first time
            create_main_ui(app);
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Apply CSS styling
static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);
    
    const char *css_data =
        "window {"
        "  background-color: #2A1F2D;"
        "}"
        "#side-panel {"
        "  background-color: #2A1F2D;"
        "  padding: 10px;"
        "}"
        "#main-view {"
        "  background-color: #2A1F2D;"
        "  padding: 10px;"
        "}"
        "#image-container {"
        "  background-color: #3B2C35;"
        "}"
        "#image-viewport {"
        "  background-color: #3B2C35;"
        "}"
        "#console-frame {"
        "  background-color: #2A1F2D;"
        "}"
        "#console-text {"
        "  background-color: #1a1520;"
        "  color: #00ff00;"
        "  font-family: monospace;"
        "  font-size: 11px;"
        "  padding: 5px;"
        "}"
        "#console-text text {"
        "  background-color: #1a1520;"
        "  color: #00ff00;"
        "}"
        "#title-label {"
        "  font-size: 48px;"
        "  font-weight: bold;"
        "  color: #FFFFFF;"
        "}";
    
    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// Main window setup
static void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Apply custom CSS styling
    apply_css();
    
    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "OCR Word Search");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 800);
    gtk_window_set_resizable(GTK_WINDOW(app->window), TRUE);
    
    // Main container
    app->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(app->main_box), 10);
    gtk_container_add(GTK_CONTAINER(app->window), app->main_box);
    
    // Title label
    app->title_label = gtk_label_new("OCR WORD SEARCH");
    gtk_widget_set_name(app->title_label, "title-label");
    gtk_widget_set_halign(app->title_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(app->title_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(app->main_box), app->title_label, FALSE, FALSE, 10);
    
    // Initial "Load Image" button
    app->load_button = gtk_button_new_with_label("Load image to start");
    gtk_widget_set_size_request(app->load_button, 200, 100);
    gtk_widget_set_halign(app->load_button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(app->load_button, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(app->main_box), app->load_button, TRUE, TRUE, 0);
    g_signal_connect(app->load_button, "clicked", G_CALLBACK(load_image_clicked), app);
    
    gtk_widget_show_all(app->window);
}

int main(int argc, char *argv[]) {
    AppData app = {0};
    
    // Initialize all output directories at startup
    initialize_output_directories();
    
    GtkApplication *gtk_app = gtk_application_new("com.ocr.wordsearch", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(activate), &app);
    
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    
    // Cleanup
    if (app.input_image_path) {
        g_free(app.input_image_path);
    }
    if (app.current_image_path) {
        g_free(app.current_image_path);
    }
    if (app.current_pixbuf) {
        g_object_unref(app.current_pixbuf);
    }
    if (app.original_pixbuf) {
        g_object_unref(app.original_pixbuf);
    }
    
    g_object_unref(gtk_app);
    return status;
}
