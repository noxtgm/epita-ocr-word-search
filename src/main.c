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
    GtkWidget *image_display;
    GtkWidget *run_button;
    GtkWidget *console_view;
    GtkTextBuffer *console_buffer;
    GtkWidget *step_buttons[NUM_STEPS];
    
    char *input_image_path;
    char *current_image_path;  // Current image being displayed (original or rotated)
    StepType current_step;
    gboolean steps_completed[NUM_STEPS];
    
    GdkPixbuf *current_pixbuf;
} AppData;

static const char *step_names[] = {
    "1. Import Image",
    "2. Rotation",
    "3. Detection",
    "4. Identify Characters",
    "5. Solve"
};

// Function prototypes
static void load_image_clicked(GtkWidget *widget, gpointer data);
static void step_clicked(GtkWidget *widget, gpointer data);
static void run_step_clicked(GtkWidget *widget, gpointer data);
static void display_image(AppData *app, const char *image_path);
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

// Initialize all output directories required by the application
static void initialize_output_directories(void) {
    // Create base outputs directory
    mkdir("../outputs", 0755);
    
    // Create subdirectories for each module
    mkdir("../outputs/rotation", 0755);
    mkdir("../outputs/grid_detection", 0755);
    mkdir("../outputs/grid_detection/cells", 0755);
    mkdir("../outputs/list_detection", 0755);
    mkdir("../outputs/recognized_files", 0755);
}

// Clean all output directories (remove contents but keep directories)
static void clean_output_directories(void) {
    system("rm -rf ../outputs/rotation/* 2>/dev/null");
    system("rm -rf ../outputs/grid_detection/* 2>/dev/null");
    system("rm -rf ../outputs/list_detection/* 2>/dev/null");
    system("rm -rf ../outputs/recognized_files/* 2>/dev/null");
    
    // Recreate subdirectories that were removed by wildcard deletion
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
    
    // Scale image to fit display area (max 790x590, accounting for 10px padding)
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    
    int max_width = 790;
    int max_height = 590;
    
    if (width > max_width || height > max_height) {
        double scale = 1.0;
        double scale_w = (double)max_width / width;
        double scale_h = (double)max_height / height;
        scale = (scale_w < scale_h) ? scale_w : scale_h;
        
        int new_width = (int)(width * scale);
        int new_height = (int)(height * scale);
        
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, new_width, new_height, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = scaled;
    }
    
    if (app->current_pixbuf) {
        g_object_unref(app->current_pixbuf);
    }
    app->current_pixbuf = pixbuf;
    
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_display), pixbuf);
    
    // Update current_image_path to track which image is being used
    if (app->current_image_path) {
        g_free(app->current_image_path);
    }
    app->current_image_path = g_strdup(image_path);
}

// Reset workflow and clean modules
static void reset_workflow(AppData *app) {
    log_message(app, "Resetting workflow...");
    
    // Clear all step completions except import
    for (int i = 1; i < NUM_STEPS; i++) {
        app->steps_completed[i] = FALSE;
    }
    
    // Clean all module outputs
    log_message(app, "Cleaning module outputs...");
    system("cd detection && make clean 2>&1");
    system("cd neural_network && make clean 2>&1");
    system("cd rotation && make clean 2>&1");
    system("cd solver && make clean 2>&1");
    
    log_message(app, "Workflow reset complete. Ready to process new image.");
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
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Load New Image");
    } else if (app->steps_completed[app->current_step]) {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Re-run Step");
    } else {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Run Step");
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
            
            // If re-running, prompt for new image
            if (app->steps_completed[STEP_IMPORT]) {
                log_message(app, "Loading new image...");
                load_image_clicked(NULL, app);
                return;
            }
            
            // First time: just display the loaded image
            app->steps_completed[STEP_IMPORT] = TRUE;
            display_image(app, app->input_image_path);
            log_message(app, "Image imported successfully");
            break;
            
        case STEP_ROTATION:
            log_message(app, "Running automatic rotation detection...");
            
            // Build rotation_detector (suppress output)
            system("cd rotation && make 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Run rotation_detector and capture output
            snprintf(command, sizeof(command), 
                     "cd rotation && ./rotation_detector \"%s\" 2>&1", 
                     app->input_image_path);
            
            fp = popen(command, "r");
            if (fp) {
                char line[256];
                while (fgets(line, sizeof(line), fp)) {
                    // Remove newline
                    line[strcspn(line, "\n")] = 0;
                    if (strlen(line) > 0) {
                        log_message(app, line);
                    }
                }
                pclose(fp);
            }
            
            // Check if rotated image was created (same name as original, in outputs/rotation)
            const char *basename = strrchr(app->input_image_path, '/');
            basename = basename ? basename + 1 : app->input_image_path;
            
            snprintf(output_path, sizeof(output_path), 
                     "../outputs/rotation/%s", basename);
            
            if (file_exists(output_path)) {
                app->steps_completed[STEP_ROTATION] = TRUE;
                display_image(app, output_path);
                log_message(app, "Rotation completed - displaying corrected image");
            } else {
                app->steps_completed[STEP_ROTATION] = TRUE;
                log_message(app, "No rotation needed - skipping this step");
            }
            break;
            
        case STEP_DETECTION:
            log_message(app, "Running letter detection...");
            
            // Check if rotated image exists (same name in outputs/rotation)
            const char *basename_det = strrchr(app->input_image_path, '/');
            basename_det = basename_det ? basename_det + 1 : app->input_image_path;
            
            char rotated_path[2048];
            snprintf(rotated_path, sizeof(rotated_path), "../outputs/rotation/%s", basename_det);
            
            // Use rotated image if it exists, otherwise use original
            const char *detect_image;
            if (file_exists(rotated_path)) {
                detect_image = rotated_path;
                log_message(app, "Using rotated image for detection");
            } else {
                detect_image = app->input_image_path;
                log_message(app, "Using original image for detection");
            }
            
            // Convert relative path to absolute path
            char absolute_path[4096];
            if (detect_image[0] != '/') {
                // Relative path - resolve it
                char cwd[1024];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    snprintf(absolute_path, sizeof(absolute_path), "%s/%s", cwd, detect_image);
                    detect_image = absolute_path;
                }
            }
            
            // Build detection (suppress output)
            system("cd detection && make 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Run detection and capture output
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
                display_image(app, "../outputs/grid_detection/debug.png");
                log_message(app, "Detection completed - displaying detected letters");
            } else {
                log_message(app, "Error: Detection failed - debug image not found");
            }
            break;
            
        case STEP_OCR:
            log_message(app, "Running OCR to identify characters...");
            
            // Build OCR (suppress output)
            system("cd neural_network && make recognize 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Run OCR recognition (already built by make recognize target)
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
                
                // Display recognized grid content
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
                
                log_message(app, "OCR completed successfully!");
            } else {
                log_message(app, "Error: OCR failed - output files not found");
            }
            break;
            
        case STEP_SOLVE:
            log_message(app, "Solving word search...");
            
            // Build solver (suppress build messages)
            system("cd solver && make 2>&1 | grep -v 'gcc\\|Entering\\|Leaving\\|make\\[\\|Nothing to be done' >/dev/null");
            
            // Read word list and solve for each word
            FILE *words_file = fopen("../outputs/recognized_files/recognized_words.txt", "r");
            if (!words_file) {
                log_message(app, "Error: Could not open word list file");
                break;
            }
            
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
            snprintf(summary, sizeof(summary), "Solved: %d/%d words found", found_count, total_count);
            log_message(app, summary);
            
            app->steps_completed[STEP_SOLVE] = TRUE;
            log_message(app, "Word search solving completed!");
            break;
    }
    
    update_step_buttons(app);
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
    
    // Display appropriate image for completed steps
    if (app->steps_completed[app->current_step]) {
        char output_path[512];
        switch (app->current_step) {
            case STEP_IMPORT:
                if (app->input_image_path) {
                    display_image(app, app->input_image_path);
                }
                break;
                
            case STEP_ROTATION:
                // Check for corrected image
                {
                    const char *basename = strrchr(app->input_image_path, '/');
                    basename = basename ? basename + 1 : app->input_image_path;
                    const char *ext = strrchr(basename, '.');
                    size_t basename_len = ext ? (size_t)(ext - basename) : strlen(basename);
                    
                    snprintf(output_path, sizeof(output_path), 
                             "%.*s_corrected.png", (int)basename_len, basename);
                    
                    if (file_exists(output_path)) {
                        display_image(app, output_path);
                    } else {
                        display_image(app, app->input_image_path);
                    }
                }
                break;
                
            case STEP_DETECTION:
                if (file_exists("../outputs/grid_detection/debug.png")) {
                    display_image(app, "../outputs/grid_detection/debug.png");
                }
                break;
                
            default:
                break;
        }
    }
    
    update_step_buttons(app);
}

// Create the main UI after image is loaded
static void create_main_ui(AppData *app) {
    // Remove load button
    gtk_widget_destroy(app->load_button);
    
    // Create horizontal box for left pane and main area
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(app->main_box), hbox, TRUE, TRUE, 0);
    
    // Left pane - step buttons
    GtkWidget *left_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(left_pane, 200, -1);
    gtk_widget_set_name(left_pane, "side-panel");
    gtk_box_pack_start(GTK_BOX(hbox), left_pane, FALSE, FALSE, 0);
    
    // Create step buttons
    for (int i = 0; i < NUM_STEPS; i++) {
        app->step_buttons[i] = gtk_button_new_with_label(step_names[i]);
        gtk_box_pack_start(GTK_BOX(left_pane), app->step_buttons[i], FALSE, FALSE, 0);
        g_signal_connect(app->step_buttons[i], "clicked", G_CALLBACK(step_clicked), app);
    }
    
    // Run button at bottom of left pane
    app->run_button = gtk_button_new_with_label("Run Step");
    gtk_box_pack_end(GTK_BOX(left_pane), app->run_button, FALSE, FALSE, 10);
    g_signal_connect(app->run_button, "clicked", G_CALLBACK(run_step_clicked), app);
    
    // Main display area
    GtkWidget *right_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_name(right_pane, "main-view");
    gtk_box_pack_start(GTK_BOX(hbox), right_pane, TRUE, TRUE, 0);
    
    // Image display with scrolled window
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
                                   GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    gtk_widget_set_name(scrolled, "image-container");
    gtk_box_pack_start(GTK_BOX(right_pane), scrolled, TRUE, TRUE, 0);
    
    // Add viewport with padding for the image
    GtkWidget *viewport = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_name(viewport, "image-viewport");
    gtk_container_add(GTK_CONTAINER(scrolled), viewport);
    
    app->image_display = gtk_image_new();
    gtk_widget_set_margin_top(app->image_display, 5);
    gtk_widget_set_margin_bottom(app->image_display, 5);
    gtk_widget_set_margin_start(app->image_display, 5);
    gtk_widget_set_margin_end(app->image_display, 5);
    gtk_container_add(GTK_CONTAINER(viewport), app->image_display);
    
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
    
    // Add image file filter
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
        "}";
    
    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(screen,
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// Main window setup
static void activate(GtkApplication *gtk_app, gpointer user_data) {
    AppData *app = (AppData *)user_data;
    
    // Apply custom CSS styling
    apply_css();
    
    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "OCR Word Search");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1000, 700);
    
    // Main container
    app->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(app->main_box), 10);
    gtk_container_add(GTK_CONTAINER(app->window), app->main_box);
    
    // Initial "Load Image" button
    app->load_button = gtk_button_new_with_label("Load Image to Start");
    gtk_widget_set_size_request(app->load_button, 200, 100);
    gtk_box_pack_start(GTK_BOX(app->main_box), app->load_button, TRUE, FALSE, 0);
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
    
    g_object_unref(gtk_app);
    return status;
}
