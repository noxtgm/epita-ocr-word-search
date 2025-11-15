#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct {
    int input_size;
    int hidden_size;
    int output_size;

    double *w_input_hidden;
    double *b_hidden;

    double *w_hidden_output;
    double *b_output;
} MLP;


// Generate a random weight between -1 and +1
double rand_weight();

// Sigmoid activation function
double sigmoid(double x);

// Derivative of the sigmoid function (y = sigmoid(x))
double sigmoid_derivative(double y);

// Softmax activation function
void softmax(double *z, double *out, int size)

// Check if a file exists
int file_exists(const char *path);

// Create and initialize a new MLP
MLP *mlp_create(int input_size, int hidden_size, int output_size);

// Free all memory used by the MLP
void mlp_free(MLP *m);

// Forward propagation: compute the output for a given input
void mlp_forward(MLP *m, double *input, double *hidden, double *output);

// Train the MLP using backpropagation
void mlp_train(MLP *m, double X[][2], double Y[][1], int samples, int epochs, double lr);

// Predict output for new input data
void mlp_predict(MLP *m, double *input, double *output);

// Save model weights and biases to a binary file
void mlp_save(MLP *m, const char *path);

// Load a model from a binary file
MLP *mlp_load(const char *path);

#endif
