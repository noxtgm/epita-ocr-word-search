#define _POSIX_C_SOURCE 200809L
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define NUM_STEPS 6

typedef enum {
    STEP_INPUT = 0,
    STEP_ROTATION,
    STEP_GRID_DETECTION,
    STEP_LIST_DETECTION,
    STEP_NEURAL_NETWORK,
    STEP_SOLVER
} StepType;

typedef struct {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *load_button;
    GtkWidget *step_box;
    GtkWidget *image_display;
    GtkWidget *run_button;
    GtkWidget *status_label;
    GtkWidget *step_buttons[NUM_STEPS];
    
    char *input_image_path;
    char *current_output_path;
    StepType current_step;
    gboolean steps_completed[NUM_STEPS];
    
    GdkPixbuf *current_pixbuf;
} AppData;

static const char *step_names[] = {
    "1. Load Image",
    "2. Rotation",
    "3. Grid Detection",
    "4. List Detection",
    "5. Identify Caracters",
    "6. Solve Word Search"
};

// Function prototypes
static void load_image_clicked(GtkWidget *widget, gpointer data);
static void step_clicked(GtkWidget *widget, gpointer data);
static void run_step_clicked(GtkWidget *widget, gpointer data);
static void display_image(AppData *app, const char *image_path);
static void update_step_buttons(AppData *app);
static void create_main_ui(AppData *app);
static void update_status(AppData *app, const char *message);

// Check if file exists
static gboolean file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

// Update status message
static void update_status(AppData *app, const char *message) {
    gtk_label_set_text(GTK_LABEL(app->status_label), message);
}

// Display image in the main area
static void display_image(AppData *app, const char *image_path) {
    if (!file_exists(image_path)) {
        update_status(app, "Error: Image file not found");
        return;
    }
    
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(image_path, &error);
    
    if (error) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Error loading image: %s", error->message);
        update_status(app, msg);
        g_error_free(error);
        return;
    }
    
    // Scale image to fit display area (max 800x600)
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    
    int max_width = 800;
    int max_height = 600;
    
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
    
    if (app->current_output_path) {
        free(app->current_output_path);
    }
    app->current_output_path = strdup(image_path);
}

// Update step button states
static void update_step_buttons(AppData *app) {
    for (int i = 0; i < NUM_STEPS; i++) {
        GtkWidget *button = app->step_buttons[i];
        GtkStyleContext *context = gtk_widget_get_style_context(button);
        
        // Remove all style classes
        gtk_style_context_remove_class(context, "suggested-action");
        gtk_style_context_remove_class(context, "destructive-action");
        
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
    
    // Update run button text
    if (app->steps_completed[app->current_step]) {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Re-run Step");
    } else {
        gtk_button_set_label(GTK_BUTTON(app->run_button), "Run Step");
    }
}

// Run the current step
static void run_step_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;  // Unused parameter
    AppData *app = (AppData *)data;
    
    if (!app->input_image_path) {
        update_status(app, "Please load an image first");
        return;
    }
    
    char command[1024];
    char output_path[512];
    
    update_status(app, "Running step...");
    
    switch (app->current_step) {
        case STEP_INPUT:
            // Input step is just displaying the loaded image
            app->steps_completed[STEP_INPUT] = TRUE;
            display_image(app, app->input_image_path);
            update_status(app, "Input image loaded successfully");
            break;
            
        case STEP_ROTATION:
            // Run rotation: ./rotate <image> <angle>
            snprintf(command, sizeof(command), 
                     "cd ../rotation && make && ./rotate %s 0", 
                     app->input_image_path);
            system(command);
            
            // Find the output image
            snprintf(output_path, sizeof(output_path), 
                     "../../outputs/rotation/rotated_%s", 
                     strrchr(app->input_image_path, '/') + 1);
            
            if (file_exists(output_path)) {
                app->steps_completed[STEP_ROTATION] = TRUE;
                display_image(app, output_path);
                update_status(app, "Rotation completed successfully");
            } else {
                update_status(app, "Rotation failed - output not found");
            }
            break;
            
        case STEP_GRID_DETECTION:
            // Run grid detection: ./detect_grid <image>
            snprintf(command, sizeof(command), 
                     "cd ../detection && make && ./detect %s", 
                     app->current_output_path);
            system(command);
            
            // Check if output exists
            if (file_exists("../../outputs/grid_detection/index.txt")) {
                app->steps_completed[STEP_GRID_DETECTION] = TRUE;
                update_status(app, "Grid detection completed - check outputs/grid_detection/");
            } else {
                update_status(app, "Grid detection failed");
            }
            break;
            
        case STEP_LIST_DETECTION:
            // List detection requires grid coordinates
            update_status(app, "List detection requires manual grid coordinates from previous step");
            app->steps_completed[STEP_LIST_DETECTION] = TRUE;
            break;
            
        case STEP_NEURAL_NETWORK:
            // Neural network training/recognition
            snprintf(command, sizeof(command), 
                     "cd ../neural_network && make train");
            system(command);
            app->steps_completed[STEP_NEURAL_NETWORK] = TRUE;
            update_status(app, "Neural network training completed");
            break;
            
        case STEP_SOLVER:
            // Solver requires grid text and words
            update_status(app, "Solver requires text grid and words from recognition step");
            app->steps_completed[STEP_SOLVER] = TRUE;
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
    
    // If step is completed, display its output
    if (app->steps_completed[app->current_step]) {
        char output_path[512];
        switch (app->current_step) {
            case STEP_INPUT:
                if (app->input_image_path) {
                    display_image(app, app->input_image_path);
                }
                break;
                
            case STEP_ROTATION:
                snprintf(output_path, sizeof(output_path), 
                         "../../outputs/rotation/rotated_%s", 
                         strrchr(app->input_image_path, '/') + 1);
                if (file_exists(output_path)) {
                    display_image(app, output_path);
                }
                break;
                
            default:
                update_status(app, "Output display for this step not yet implemented");
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
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_name(scrolled, "image-container");
    gtk_box_pack_start(GTK_BOX(right_pane), scrolled, TRUE, TRUE, 0);
    
    app->image_display = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(scrolled), app->image_display);
    
    // Status bar
    app->status_label = gtk_label_new("Ready");
    gtk_box_pack_end(GTK_BOX(app->main_box), app->status_label, FALSE, FALSE, 5);
    
    // Set initial step to INPUT
    app->current_step = STEP_INPUT;
    app->steps_completed[STEP_INPUT] = TRUE;
    
    // Display the loaded image
    display_image(app, app->input_image_path);
    
    update_step_buttons(app);
    gtk_widget_show_all(app->window);
}

// Handle image load button click
static void load_image_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;  // Unused parameter
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
            free(app->input_image_path);
        }
        app->input_image_path = filename;
        
        // Create main UI
        create_main_ui(app);
    }
    
    gtk_widget_destroy(dialog);
}

// Apply CSS styling
static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);
    
    const char *css_data =
        "#side-panel {"
        "  background-color: #2A1F2D;"
        "  padding: 10px;"
        "}"
        "#main-view {"
        "  background-color: #3B2C35;"
        "}"
        "#image-container {"
        "  background-color: #3B2C35;"
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
    app->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
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
    
    GtkApplication *gtk_app = gtk_application_new("com.ocr.wordsearch", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(activate), &app);
    
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    
    // Cleanup
    if (app.input_image_path) {
        free(app.input_image_path);
    }
    if (app.current_output_path) {
        free(app.current_output_path);
    }
    if (app.current_pixbuf) {
        g_object_unref(app.current_pixbuf);
    }
    
    g_object_unref(gtk_app);
    return status;
}

