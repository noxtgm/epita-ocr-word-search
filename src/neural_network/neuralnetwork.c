#include "neuralnetwork.h"

// -----------------------------
//  Utility Functions
// -----------------------------

double rand_weight_scaled(int fan_in) {
    return (((double)rand() / RAND_MAX) * 2 - 1) * sqrt(1.0 / fan_in);
}

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double sigmoid_derivative(double y) {
    return y * (1.0 - y);
}

void softmax(double *z, double *out, int size) {
    double max = z[0];
    for (int i = 1; i < size; i++)
        if (z[i] > max) max = z[i];
    
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        out[i] = exp(z[i] - max);
        sum += out[i];
    }
    for (int i = 0; i < size; i++)
        out[i] /= sum;
}

// -----------------------------
//  MLP Functions
// -----------------------------

MLP *mlp_create(int input_size, int hidden_size, int output_size) {
    MLP *m = malloc(sizeof(MLP));
    m->input_size = input_size;
    m->hidden_size = hidden_size;
    m->output_size = output_size;
    
    m->w_input_hidden = malloc(sizeof(double) * input_size * hidden_size);
    m->b_hidden = malloc(sizeof(double) * hidden_size);
    m->w_hidden_output = malloc(sizeof(double) * hidden_size * output_size);
    m->b_output = malloc(sizeof(double) * output_size);
    
    for (int i = 0; i < input_size * hidden_size; i++)
        m->w_input_hidden[i] = rand_weight_scaled(input_size);
    
    for (int i = 0; i < hidden_size; i++)
        m->b_hidden[i] = rand_weight_scaled(input_size);
    
    for (int i = 0; i < hidden_size * output_size; i++)
        m->w_hidden_output[i] = rand_weight_scaled(hidden_size);
    
    for (int i = 0; i < output_size; i++)
        m->b_output[i] = rand_weight_scaled(hidden_size);
    
    return m;
}

void mlp_free(MLP *m) {
    free(m->w_input_hidden);
    free(m->b_hidden);
    free(m->w_hidden_output);
    free(m->b_output);
    free(m);
}

void mlp_forward(MLP *m, double *input, double *hidden, double *output) {
    for (int j = 0; j < m->hidden_size; j++) {
        double sum = m->b_hidden[j];
        for (int i = 0; i < m->input_size; i++)
            sum += input[i] * m->w_input_hidden[j * m->input_size + i];
        hidden[j] = sigmoid(sum);
    }
    
    double *z = malloc(sizeof(double) * m->output_size);
    for (int k = 0; k < m->output_size; k++) {
        double sum = m->b_output[k];
        for (int j = 0; j < m->hidden_size; j++)
            sum += hidden[j] * m->w_hidden_output[k * m->hidden_size + j];
        z[k] = sum;
    }
    
    softmax(z, output, m->output_size);
    free(z);
}

void mlp_predict(MLP *m, double *input, double *output) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    mlp_forward(m, input, hidden, output);
    free(hidden);
}

void mlp_save(MLP *m, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("Failed to save model");
        return;
    }
    
    fwrite(&m->input_size, sizeof(int), 1, f);
    fwrite(&m->hidden_size, sizeof(int), 1, f);
    fwrite(&m->output_size, sizeof(int), 1, f);
    
    fwrite(m->w_input_hidden, sizeof(double), m->input_size * m->hidden_size, f);
    fwrite(m->b_hidden, sizeof(double), m->hidden_size, f);
    fwrite(m->w_hidden_output, sizeof(double), m->hidden_size * m->output_size, f);
    fwrite(m->b_output, sizeof(double), m->output_size, f);
    
    fclose(f);
}

// -----------------------------
//  Image Loading
// -----------------------------

double* load_image(const char *path) {
    GError *error = NULL;
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &error);
    if (!pixbuf) {
        fprintf(stderr, "Failed to load %s: %s\n", path, error->message);
        g_error_free(error);
        return NULL;
    }
    
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, TARGET_WIDTH, TARGET_HEIGHT, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    
    if (!scaled) return NULL;
    
    int n_channels = gdk_pixbuf_get_n_channels(scaled);
    int rowstride = gdk_pixbuf_get_rowstride(scaled);
    guchar *pixels = gdk_pixbuf_get_pixels(scaled);
    
    double *normalized = malloc(sizeof(double) * INPUT_SIZE);
    
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            guchar *pixel = pixels + y * rowstride + x * n_channels;
            
            double gray;
            if (n_channels >= 3) {
                gray = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
            } else {
                gray = pixel[0];
            }
            
            normalized[y * TARGET_WIDTH + x] = gray / 255.0;
        }
    }
    
    g_object_unref(scaled);
    return normalized;
}

// -----------------------------
//  Dataset
// -----------------------------

Dataset* dataset_create(int capacity) {
    Dataset *ds = malloc(sizeof(Dataset));
    ds->inputs = malloc(sizeof(double*) * capacity);
    ds->labels = malloc(sizeof(double*) * capacity);
    ds->count = 0;
    ds->capacity = capacity;
    return ds;
}

void dataset_free(Dataset *ds) {
    for (int i = 0; i < ds->count; i++) {
        free(ds->inputs[i]);
        free(ds->labels[i]);
    }
    free(ds->inputs);
    free(ds->labels);
    free(ds);
}

void dataset_add(Dataset *ds, double *input, int label_index) {
    if (ds->count >= ds->capacity) return;
    
    ds->inputs[ds->count] = input;
    
    ds->labels[ds->count] = calloc(OUTPUT_SIZE, sizeof(double));
    ds->labels[ds->count][label_index] = 1.0;
    
    ds->count++;
}

void dataset_shuffle(Dataset *ds) {
    for (int i = ds->count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        
        double *temp_input = ds->inputs[i];
        ds->inputs[i] = ds->inputs[j];
        ds->inputs[j] = temp_input;
        
        double *temp_label = ds->labels[i];
        ds->labels[i] = ds->labels[j];
        ds->labels[j] = temp_label;
    }
}

// Simple way to read directory - reads file list from stdin
Dataset* load_dataset_from_list() {
    Dataset *ds = dataset_create(10000);
    
    printf("Enter image paths (one per line, format: <letter> <path>)\n");
    printf("Example: A labeled_cells/A/cell1.png\n");
    printf("Press Ctrl+D when done:\n\n");
    
    char letter;
    char path[1024];
    
    while (scanf(" %c %s", &letter, path) == 2) {
        if (letter < 'A' || letter > 'Z') continue;
        
        double *img_data = load_image(path);
        if (img_data) {
            dataset_add(ds, img_data, letter - 'A');
            printf("Loaded: %c - %s\n", letter, path);
        }
    }
    
    printf("\nTotal loaded: %d images\n", ds->count);
    return ds;
}

// Load from file list instead of directory scanning
Dataset* load_dataset_from_file(const char *list_file) {
    Dataset *ds = dataset_create(10000);
    
    FILE *f = fopen(list_file, "r");
    if (!f) {
        perror("Failed to open file list");
        return ds;
    }
    
    char letter;
    char path[1024];
    
    while (fscanf(f, " %c %s", &letter, path) == 2) {
        if (letter < 'A' || letter > 'Z') continue;
        
        double *img_data = load_image(path);
        if (img_data) {
            dataset_add(ds, img_data, letter - 'A');
        }
    }
    
    fclose(f);
    printf("Loaded %d images from %s\n", ds->count, list_file);
    return ds;
}

// -----------------------------
//  Training
// -----------------------------

double evaluate_accuracy(MLP *m, Dataset *ds) {
    double *output = malloc(sizeof(double) * m->output_size);
    int correct = 0;
    
    for (int i = 0; i < ds->count; i++) {
        mlp_predict(m, ds->inputs[i], output);
        
        int predicted = 0, actual = 0;
        for (int j = 1; j < m->output_size; j++) {
            if (output[j] > output[predicted]) predicted = j;
            if (ds->labels[i][j] > ds->labels[i][actual]) actual = j;
        }
        
        if (predicted == actual) correct++;
    }
    
    free(output);
    return (double)correct / ds->count * 100.0;
}

void mlp_train(MLP *m, Dataset *train, Dataset *val, int epochs, double lr, const char *save_path) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    double *output = malloc(sizeof(double) * m->output_size);
    double best_val_acc = 0.0;
    
    for (int e = 0; e < epochs; e++) {
        dataset_shuffle(train);
        double total_error = 0.0;
        
        for (int n = 0; n < train->count; n++) {
            mlp_forward(m, train->inputs[n], hidden, output);
            
            double delta_out[m->output_size];
            for (int k = 0; k < m->output_size; k++) {
                delta_out[k] = output[k] - train->labels[n][k];
                total_error += -train->labels[n][k] * log(output[k] + 1e-12);
            }
            
            for (int k = 0; k < m->output_size; k++) {
                for (int j = 0; j < m->hidden_size; j++)
                    m->w_hidden_output[k * m->hidden_size + j] -= lr * delta_out[k] * hidden[j];
                m->b_output[k] -= lr * delta_out[k];
            }
            
            double delta_hidden[m->hidden_size];
            for (int j = 0; j < m->hidden_size; j++) {
                double sum = 0.0;
                for (int k = 0; k < m->output_size; k++)
                    sum += delta_out[k] * m->w_hidden_output[k * m->hidden_size + j];
                delta_hidden[j] = sum * sigmoid_derivative(hidden[j]);
            }
            
            for (int j = 0; j < m->hidden_size; j++) {
                for (int i = 0; i < m->input_size; i++)
                    m->w_input_hidden[j * m->input_size + i] -= lr * delta_hidden[j] * train->inputs[n][i];
                m->b_hidden[j] -= lr * delta_hidden[j];
            }
        }
        
        if (e % 10 == 0) {
            double train_acc = evaluate_accuracy(m, train);
            double val_acc = evaluate_accuracy(m, val);
            
            printf("Epoch %4d | Loss: %.4f | Train: %.2f%% | Val: %.2f%%\n",
                   e, total_error / train->count, train_acc, val_acc);
            
            if (val_acc > best_val_acc) {
                best_val_acc = val_acc;
                mlp_save(m, save_path);
                printf("  -> Saved best model (%.2f%%)\n", val_acc);
            }
        }
    }
    
    free(hidden);
    free(output);
}

// -----------------------------
//  Main
// -----------------------------

int main(int argc, char **argv) {
    srand(42);
    
    if (argc < 2) {
        printf("Usage: %s <file_list.txt> [output_model.bin]\n", argv[0]);
        printf("\nfile_list.txt format (one per line):\n");
        printf("  A labeled_cells/A/cell1.png\n");
        printf("  A labeled_cells/A/cell2.png\n");
        printf("  B labeled_cells/B/cell1.png\n");
        printf("  ...\n");
        return 1;
    }
    
    const char *list_file = argv[1];
    const char *model_path = (argc > 2) ? argv[2] : "letter_model.bin";
    
    printf("Loading dataset from: %s\n", list_file);
    Dataset *full = load_dataset_from_file(list_file);
    
    if (full->count < 10) {
        fprintf(stderr, "Not enough data! Need at least 10 images.\n");
        return 1;
    }
    
    dataset_shuffle(full);
    int train_size = (int)(full->count * 0.8);
    
    Dataset *train = dataset_create(train_size);
    Dataset *val = dataset_create(full->count - train_size);
    
    for (int i = 0; i < train_size; i++) {
        train->inputs[i] = full->inputs[i];
        train->labels[i] = full->labels[i];
        train->count++;
    }
    
    for (int i = train_size; i < full->count; i++) {
        val->inputs[i - train_size] = full->inputs[i];
        val->labels[i - train_size] = full->labels[i];
        val->count++;
    }
    
    printf("\nTrain: %d | Val: %d\n\n", train->count, val->count);
    
    printf("Creating model...\n");
    MLP *model = mlp_create(INPUT_SIZE, HIDDEN_SIZE, OUTPUT_SIZE);
    
    printf("Training...\n");
    mlp_train(model, train, val, 500, 0.01, model_path);
    
    printf("\n=== Final Results ===\n");
    printf("Train Accuracy: %.2f%%\n", evaluate_accuracy(model, train));
    printf("Val Accuracy: %.2f%%\n", evaluate_accuracy(model, val));
    printf("Model saved to: %s\n", model_path);
    
    mlp_free(model);
    dataset_free(train);
    dataset_free(val);
    free(full);
    
    return 0;
}
