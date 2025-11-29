#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>

// Neural network configuration
#define TARGET_WIDTH 28      // Input image width
#define TARGET_HEIGHT 28     // Input image height
#define INPUT_SIZE (TARGET_WIDTH * TARGET_HEIGHT)  // Total input neurons (784)
#define HIDDEN_SIZE 128      // Hidden layer neurons
#define OUTPUT_SIZE 26       // Output classes (A-Z)

// Multi-Layer Perceptron structure
typedef struct {
    int input_size;
    int hidden_size;
    int output_size;

    double *w_input_hidden;   // Weights: input -> hidden
    double *b_hidden;         // Biases: hidden layer

    double *w_hidden_output;  // Weights: hidden -> output
    double *b_output;         // Biases: output layer
} MLP;

// Training dataset structure
typedef struct {
    double **inputs;   // Array of input images
    double **labels;   // Array of one-hot encoded labels
    int count;         // Current number of samples
    int capacity;      // Maximum capacity
} Dataset;

// Activation functions
double rand_weight_scaled(int fan_in);      // Xavier/He initialization
double sigmoid(double x);                    // Sigmoid activation
double sigmoid_derivative(double y);         // Sigmoid derivative
void softmax(double *z, double *out, int size);  // Softmax for output layer

// Neural network operations
MLP *mlp_create(int input_size, int hidden_size, int output_size);  // Create network
void mlp_free(MLP *m);                                               // Free memory
void mlp_forward(MLP *m, double *input, double *hidden, double *output);  // Forward pass
void mlp_predict(MLP *m, double *input, double *output);            // Prediction only
void mlp_save(MLP *m, const char *path);                            // Save model to file
MLP *mlp_load(const char *path);                                    // Load model from file

// Image preprocessing
double* load_image(const char *path);  // Load and normalize image to 28x28

// Dataset management
Dataset* dataset_create(int capacity);                      // Create empty dataset
void dataset_free(Dataset *ds);                            // Free dataset memory
void dataset_add(Dataset *ds, double *input, int label_index);  // Add sample
void dataset_shuffle(Dataset *ds);                         // Shuffle for training
Dataset* load_dataset_from_file(const char *list_file);   // Load from label file

// Training utilities
double evaluate_accuracy(MLP *m, Dataset *ds);  // Calculate accuracy on dataset
void mlp_train(MLP *m, Dataset *train, Dataset *val, int epochs, double lr, const char *save_path);

#endif
