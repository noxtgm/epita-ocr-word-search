#include "enhanced_training.h"

// Enhanced preprocessing: adaptive thresholding handles varying font weights and styles
double* load_image_enhanced(const char *path) {
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
    
    // Step 1: Convert to grayscale
    double *gray = malloc(sizeof(double) * INPUT_SIZE);
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            guchar *pixel = pixels + y * rowstride + x * n_channels;
            
            if (n_channels >= 3) {
                gray[y * TARGET_WIDTH + x] = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
            } else {
                gray[y * TARGET_WIDTH + x] = pixel[0];
            }
        }
    }
    
    // Step 2: Adaptive thresholding - compares each pixel to its local neighborhood
    // This handles different font weights better than global thresholding
    int window = 5;
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            double sum = 0.0;
            int count = 0;
            
            // Calculate local mean in window around pixel
            for (int dy = -window; dy <= window; dy++) {
                for (int dx = -window; dx <= window; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (ny >= 0 && ny < TARGET_HEIGHT && nx >= 0 && nx < TARGET_WIDTH) {
                        sum += gray[ny * TARGET_WIDTH + nx];
                        count++;
                    }
                }
            }
            
            double local_mean = sum / count;
            double threshold = local_mean * 0.9;
            
            // Binary thresholding based on local mean
            normalized[y * TARGET_WIDTH + x] = (gray[y * TARGET_WIDTH + x] < threshold) ? 0.0 : 1.0;
        }
    }
    
    // Step 3: Normalize to center the data (zero mean)
    double mean = 0.0;
    for (int i = 0; i < INPUT_SIZE; i++) {
        mean += normalized[i];
    }
    mean /= INPUT_SIZE;
    
    for (int i = 0; i < INPUT_SIZE; i++) {
        normalized[i] = (normalized[i] - mean) / 0.5;
    }
    
    free(gray);
    g_object_unref(scaled);
    return normalized;
}

// Data augmentation: creates 5 variations per image (original + 4 augmentations)
AugmentedSample* augment_image(double *img, int label, int *count) {
    AugmentedSample *samples = malloc(sizeof(AugmentedSample) * 5);
    *count = 0;
    
    // 1. Original image
    samples[*count].data = malloc(sizeof(double) * INPUT_SIZE);
    memcpy(samples[*count].data, img, sizeof(double) * INPUT_SIZE);
    samples[*count].label = label;
    (*count)++;
    
    // 2. Add noise variation 1 (light noise)
    samples[*count].data = malloc(sizeof(double) * INPUT_SIZE);
    for (int i = 0; i < INPUT_SIZE; i++) {
        double noise = ((double)rand() / RAND_MAX - 0.5) * 0.1;
        samples[*count].data[i] = img[i] + noise;
    }
    samples[*count].label = label;
    (*count)++;
    
    // 3. Add noise variation 2 (moderate noise)
    samples[*count].data = malloc(sizeof(double) * INPUT_SIZE);
    for (int i = 0; i < INPUT_SIZE; i++) {
        double noise = ((double)rand() / RAND_MAX - 0.5) * 0.15;
        samples[*count].data[i] = img[i] + noise;
    }
    samples[*count].label = label;
    (*count)++;
    
    // 4. Slight shift right (simulates position variation)
    samples[*count].data = malloc(sizeof(double) * INPUT_SIZE);
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            int src_x = (x > 0) ? x - 1 : 0;
            samples[*count].data[y * TARGET_WIDTH + x] = img[y * TARGET_WIDTH + src_x];
        }
    }
    samples[*count].label = label;
    (*count)++;
    
    // 5. Slight shift left
    samples[*count].data = malloc(sizeof(double) * INPUT_SIZE);
    for (int y = 0; y < TARGET_HEIGHT; y++) {
        for (int x = 0; x < TARGET_WIDTH; x++) {
            int src_x = (x < TARGET_WIDTH - 1) ? x + 1 : TARGET_WIDTH - 1;
            samples[*count].data[y * TARGET_WIDTH + x] = img[y * TARGET_WIDTH + src_x];
        }
    }
    samples[*count].label = label;
    (*count)++;
    
    return samples;
}

// Load dataset and apply augmentation (5x data expansion)
Dataset* load_dataset_with_augmentation(const char *list_file) {
    Dataset *ds = dataset_create(50000);
    
    FILE *f = fopen(list_file, "r");
    if (!f) {
        perror("Failed to open file list");
        return ds;
    }
    
    char letter;
    char path[1024];
    int original_count = 0;
    
    while (fscanf(f, " %c %s", &letter, path) == 2) {
        if (letter < 'A' || letter > 'Z') continue;
        
        double *img_data = load_image_enhanced(path);
        if (img_data) {
            // Generate augmented versions
            int aug_count;
            AugmentedSample *augmented = augment_image(img_data, letter - 'A', &aug_count);
            
            // Add all augmented samples to dataset
            for (int i = 0; i < aug_count; i++) {
                dataset_add(ds, augmented[i].data, augmented[i].label);
            }
            
            free(augmented);
            free(img_data);
            original_count++;
            
            if (original_count % 50 == 0) {
                printf("Loaded %d images (%d total samples with augmentation)\n", 
                       original_count, ds->count);
            }
        }
    }
    
    fclose(f);
    printf("\nLoaded %d original images -> %d augmented samples\n", 
           original_count, ds->count);
    return ds;
}

// Enhanced training with regularization techniques
void mlp_train_enhanced(MLP *m, Dataset *train, Dataset *val, int epochs, double lr, const char *save_path) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    double *output = malloc(sizeof(double) * m->output_size);
    double best_val_acc = 0.0;
    int patience = 0;
    const int max_patience = 50;  // Early stopping threshold
    
    for (int e = 0; e < epochs; e++) {
        dataset_shuffle(train);
        double total_error = 0.0;
        
        // Adaptive learning rate decay
        double current_lr = lr * (1.0 / (1.0 + 0.001 * e));
        
        for (int n = 0; n < train->count; n++) {
            mlp_forward(m, train->inputs[n], hidden, output);
            
            // Apply dropout: randomly disable 20% of hidden neurons
            // This prevents overfitting by forcing network to learn redundant representations
            for (int j = 0; j < m->hidden_size; j++) {
                if ((double)rand() / RAND_MAX < 0.2) {
                    hidden[j] = 0.0;
                } else {
                    hidden[j] *= 1.25;  // Scale up to maintain expected sum
                }
            }
            
            // Compute output gradient
            double *delta_out = malloc(sizeof(double) * m->output_size);
            for (int k = 0; k < m->output_size; k++) {
                delta_out[k] = output[k] - train->labels[n][k];
                total_error += -train->labels[n][k] * log(output[k] + 1e-12);
            }
            
            // Backpropagation with L2 regularization (weight decay)
            double l2_lambda = 0.0001;
            
            // Update output layer with L2 penalty
            for (int k = 0; k < m->output_size; k++) {
                for (int j = 0; j < m->hidden_size; j++) {
                    double grad = delta_out[k] * hidden[j];
                    double reg = l2_lambda * m->w_hidden_output[k * m->hidden_size + j];
                    m->w_hidden_output[k * m->hidden_size + j] -= current_lr * (grad + reg);
                }
                m->b_output[k] -= current_lr * delta_out[k];
            }
            
            // Backpropagate to hidden layer
            double *delta_hidden = malloc(sizeof(double) * m->hidden_size);
            for (int j = 0; j < m->hidden_size; j++) {
                double sum = 0.0;
                for (int k = 0; k < m->output_size; k++)
                    sum += delta_out[k] * m->w_hidden_output[k * m->hidden_size + j];
                delta_hidden[j] = sum * sigmoid_derivative(hidden[j]);
            }
            
            // Update input layer with L2 penalty
            for (int j = 0; j < m->hidden_size; j++) {
                for (int i = 0; i < m->input_size; i++) {
                    double grad = delta_hidden[j] * train->inputs[n][i];
                    double reg = l2_lambda * m->w_input_hidden[j * m->input_size + i];
                    m->w_input_hidden[j * m->input_size + i] -= current_lr * (grad + reg);
                }
                m->b_hidden[j] -= current_lr * delta_hidden[j];
            }
            
            free(delta_out);
            free(delta_hidden);
        }
        
        // Evaluate and implement early stopping
        if (e % 5 == 0 || e == epochs - 1) {
            double train_acc = evaluate_accuracy(m, train);
            double val_acc = evaluate_accuracy(m, val);
            
            printf("Epoch %4d | Loss: %.4f | LR: %.5f | Train: %.2f%% | Val: %.2f%%\n",
                   e, total_error / train->count, current_lr, train_acc, val_acc);
            
            // Save best model based on validation accuracy
            if (val_acc > best_val_acc) {
                best_val_acc = val_acc;
                mlp_save(m, save_path);
                printf("  -> Saved best model (%.2f%%)\n", val_acc);
                patience = 0;
            } else {
                patience++;
                // Stop if validation accuracy doesn't improve
                if (patience >= max_patience) {
                    printf("\nEarly stopping: validation accuracy hasn't improved for %d epochs\n", max_patience);
                    break;
                }
            }
        }
    }
    
    free(hidden);
    free(output);
}

int main(int argc, char **argv) {
    srand(42);  // Seed for reproducibility
    
    if (argc < 2) {
        printf("Usage: %s <file_list.txt> [output_model.bin]\n", argv[0]);
        printf("\nEnhanced training with:\n");
        printf("  - Data augmentation (5x samples)\n");
        printf("  - Adaptive thresholding for multiple fonts\n");
        printf("  - Dropout regularization\n");
        printf("  - L2 weight decay\n");
        printf("  - Early stopping\n");
        return 1;
    }
    
    const char *list_file = argv[1];
    const char *model_path = (argc > 2) ? argv[2] : "letter_model.bin";
    
    printf("Loading dataset with augmentation from: %s\n", list_file);
    Dataset *full = load_dataset_with_augmentation(list_file);
    
    if (full->count < 50) {
        fprintf(stderr, "Not enough data! Need at least 50 samples.\n");
        dataset_free(full);
        return 1;
    }
    
    // Split dataset: 80% train, 20% validation
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
    
    printf("Training with enhanced methods...\n");
    mlp_train_enhanced(model, train, val, 1000, 0.01, model_path);
    
    printf("\n=== Final Results ===\n");
    printf("Train Accuracy: %.2f%%\n", evaluate_accuracy(model, train));
    printf("Val Accuracy: %.2f%%\n", evaluate_accuracy(model, val));
    printf("Model saved to: %s\n", model_path);
    
    mlp_free(model);
    
    free(train->inputs);
    free(train->labels);
    free(train);
    
    free(val->inputs);
    free(val->labels);
    free(val);
    
    dataset_free(full);
    
    return 0;
}
