#include "neuralnetwork.h"

// -----------------------------
//  Utility Functions
// -----------------------------

// Xavier/He weight initialization for better convergence
double rand_weight_scaled(int fan_in) {
    return (((double)rand() / RAND_MAX) * 2 - 1) * sqrt(1.0 / fan_in);
}

// Sigmoid activation function: output in range (0, 1)
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// Derivative of sigmoid for backpropagation
double sigmoid_derivative(double y) {
    return y * (1.0 - y);
}

// Softmax: converts logits to probability distribution
void softmax(double *z, double *out, int size) {
    double max = z[0];
    for (int i = 1; i < size; i++)
        if (z[i] > max) max = z[i];
    
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        out[i] = exp(z[i] - max);  // Subtract max for numerical stability
        sum += out[i];
    }
    for (int i = 0; i < size; i++)
        out[i] /= sum;
}

// -----------------------------
//  MLP Functions
// -----------------------------

// Create and initialize neural network with random weights
MLP *mlp_create(int input_size, int hidden_size, int output_size) {
    MLP *m = malloc(sizeof(MLP));
    m->input_size = input_size;
    m->hidden_size = hidden_size;
    m->output_size = output_size;
    
    m->w_input_hidden = malloc(sizeof(double) * input_size * hidden_size);
    m->b_hidden = malloc(sizeof(double) * hidden_size);
    m->w_hidden_output = malloc(sizeof(double) * hidden_size * output_size);
    m->b_output = malloc(sizeof(double) * output_size);
    
    // Initialize weights with scaled random values
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

// Free all allocated memory
void mlp_free(MLP *m) {
    free(m->w_input_hidden);
    free(m->b_hidden);
    free(m->w_hidden_output);
    free(m->b_output);
    free(m);
}

// Forward pass through the network
void mlp_forward(MLP *m, double *input, double *hidden, double *output) {
    // Input -> Hidden layer (with sigmoid activation)
    for (int j = 0; j < m->hidden_size; j++) {
        double sum = m->b_hidden[j];
        for (int i = 0; i < m->input_size; i++)
            sum += input[i] * m->w_input_hidden[j * m->input_size + i];
        hidden[j] = sigmoid(sum);
    }
    
    // Hidden -> Output layer (with softmax activation)
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

// Predict without exposing hidden layer
void mlp_predict(MLP *m, double *input, double *output) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    mlp_forward(m, input, hidden, output);
    free(hidden);
}

// Save model weights to binary file
void mlp_save(MLP *m, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("Failed to save model");
        return;
    }
    
    // Write architecture
    fwrite(&m->input_size, sizeof(int), 1, f);
    fwrite(&m->hidden_size, sizeof(int), 1, f);
    fwrite(&m->output_size, sizeof(int), 1, f);
    
    // Write weights and biases
    fwrite(m->w_input_hidden, sizeof(double), m->input_size * m->hidden_size, f);
    fwrite(m->b_hidden, sizeof(double), m->hidden_size, f);
    fwrite(m->w_hidden_output, sizeof(double), m->hidden_size * m->output_size, f);
    fwrite(m->b_output, sizeof(double), m->output_size, f);
    
    fclose(f);
}

// Load model from binary file
MLP *mlp_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("Failed to load model");
        return NULL;
    }
    
    // Read architecture
    int input_size, hidden_size, output_size;
    fread(&input_size, sizeof(int), 1, f);
    fread(&hidden_size, sizeof(int), 1, f);
    fread(&output_size, sizeof(int), 1, f);
    
    // Allocate memory
    MLP *m = malloc(sizeof(MLP));
    m->input_size = input_size;
    m->hidden_size = hidden_size;
    m->output_size = output_size;
    
    m->w_input_hidden = malloc(sizeof(double) * input_size * hidden_size);
    m->b_hidden = malloc(sizeof(double) * hidden_size);
    m->w_hidden_output = malloc(sizeof(double) * hidden_size * output_size);
    m->b_output = malloc(sizeof(double) * output_size);
    
    // Read weights and biases
    fread(m->w_input_hidden, sizeof(double), input_size * hidden_size, f);
    fread(m->b_hidden, sizeof(double), hidden_size, f);
    fread(m->w_hidden_output, sizeof(double), hidden_size * output_size, f);
    fread(m->b_output, sizeof(double), output_size, f);
    
    fclose(f);
    return m;
}

// -----------------------------
//  Image Loading
// -----------------------------

// Load image, resize to 28x28, convert to grayscale, normalize to [0,1]
double* load_image(const char *path) {
    GError *error = NULL;
    
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &error);
    if (!pixbuf) {
        fprintf(stderr, "Failed to load %s: %s\n", path, error->message);
        g_error_free(error);
        return NULL;
    }
    
    // Resize to target dimensions
    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, TARGET_WIDTH, TARGET_HEIGHT, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    
    if (!scaled) return NULL;
    
    int n_channels = gdk_pixbuf_get_n_channels(scaled);
    int rowstride = gdk_pixbuf_get_rowstride(scaled);
    guchar *pixels = gdk_pixbuf_get_pixels(scaled);
    
    double *normalized = malloc(sizeof(double) * INPUT_SIZE);
    
    // Convert to grayscale using luminance formula and normalize
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

// Create empty dataset with given capacity
Dataset* dataset_create(int capacity) {
    Dataset *ds = malloc(sizeof(Dataset));
    ds->inputs = malloc(sizeof(double*) * capacity);
    ds->labels = malloc(sizeof(double*) * capacity);
    ds->count = 0;
    ds->capacity = capacity;
    return ds;
}

// Free dataset and all stored samples
void dataset_free(Dataset *ds) {
    for (int i = 0; i < ds->count; i++) {
        free(ds->inputs[i]);
        free(ds->labels[i]);
    }
    free(ds->inputs);
    free(ds->labels);
    free(ds);
}

// Add sample to dataset with one-hot encoded label
void dataset_add(Dataset *ds, double *input, int label_index) {
    if (ds->count >= ds->capacity) return;
    
    ds->inputs[ds->count] = input;
    
    // Create one-hot encoded label
    ds->labels[ds->count] = calloc(OUTPUT_SIZE, sizeof(double));
    ds->labels[ds->count][label_index] = 1.0;
    
    ds->count++;
}

// Shuffle dataset using Fisher-Yates algorithm
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

// Load dataset from text file: format is "LETTER /path/to/image.png"
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
            printf("Loaded: %c - %s\n", letter, path);
        }
    }
    
    fclose(f);
    printf("\nLoaded %d images from %s\n", ds->count, list_file);
    return ds;
}

// -----------------------------
//  Training
// -----------------------------

// Calculate classification accuracy on dataset
double evaluate_accuracy(MLP *m, Dataset *ds) {
    double *output = malloc(sizeof(double) * m->output_size);
    int correct = 0;
    
    for (int i = 0; i < ds->count; i++) {
        mlp_predict(m, ds->inputs[i], output);
        
        // Find predicted and actual class
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

// Train network using stochastic gradient descent
void mlp_train(MLP *m, Dataset *train, Dataset *val, int epochs, double lr, const char *save_path) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    double *output = malloc(sizeof(double) * m->output_size);
    double best_val_acc = 0.0;
    
    for (int e = 0; e < epochs; e++) {
        dataset_shuffle(train);
        double total_error = 0.0;
        
        // Train on each sample
        for (int n = 0; n < train->count; n++) {
            mlp_forward(m, train->inputs[n], hidden, output);
            
            // Compute output layer gradient (cross-entropy loss)
            double *delta_out = malloc(sizeof(double) * m->output_size);
            for (int k = 0; k < m->output_size; k++) {
                delta_out[k] = output[k] - train->labels[n][k];
                total_error += -train->labels[n][k] * log(output[k] + 1e-12);
            }
            
            // Update output layer weights
            for (int k = 0; k < m->output_size; k++) {
                for (int j = 0; j < m->hidden_size; j++)
                    m->w_hidden_output[k * m->hidden_size + j] -= lr * delta_out[k] * hidden[j];
                m->b_output[k] -= lr * delta_out[k];
            }
            
            // Backpropagate to hidden layer
            double *delta_hidden = malloc(sizeof(double) * m->hidden_size);
            for (int j = 0; j < m->hidden_size; j++) {
                double sum = 0.0;
                for (int k = 0; k < m->output_size; k++)
                    sum += delta_out[k] * m->w_hidden_output[k * m->hidden_size + j];
                delta_hidden[j] = sum * sigmoid_derivative(hidden[j]);
            }
            
            // Update input layer weights
            for (int j = 0; j < m->hidden_size; j++) {
                for (int i = 0; i < m->input_size; i++)
                    m->w_input_hidden[j * m->input_size + i] -= lr * delta_hidden[j] * train->inputs[n][i];
                m->b_hidden[j] -= lr * delta_hidden[j];
            }
            
            free(delta_out);
            free(delta_hidden);
        }
        
        // Evaluate and save best model every 10 epochs
        if (e % 10 == 0 || e == epochs - 1) {
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
