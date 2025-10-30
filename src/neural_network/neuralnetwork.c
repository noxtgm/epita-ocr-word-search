#include "neuralnetwork.h"

// Generate a random weight between -1 and +1
double rand_weight() {
    return ((double)rand() / RAND_MAX) * 2 - 1;
}

// Sigmoid activation function
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

// Derivative of the sigmoid function (y = sigmoid(x))
double sigmoid_derivative(double y) {
    return y * (1.0 - y);
}

// Check if a file exists
int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

// Create and initialize a new MLP
MLP *mlp_create(int input_size, int hidden_size, int output_size) {
    MLP *m = malloc(sizeof(MLP)); // Allocate memory for the structure
    m->input_size = input_size;
    m->hidden_size = hidden_size;
    m->output_size = output_size;

    // Allocate memory for all weights and biases
    m->w_input_hidden = malloc(sizeof(double) * input_size * hidden_size);
    m->b_hidden = malloc(sizeof(double) * hidden_size);
    m->w_hidden_output = malloc(sizeof(double) * hidden_size * output_size);
    m->b_output = malloc(sizeof(double) * output_size);

    // Random initialization of all weights and biases
    for (int i = 0; i < input_size * hidden_size; i++) m->w_input_hidden[i] = rand_weight();
    for (int i = 0; i < hidden_size; i++) m->b_hidden[i] = rand_weight();
    for (int i = 0; i < hidden_size * output_size; i++) m->w_hidden_output[i] = rand_weight();
    for (int i = 0; i < output_size; i++) m->b_output[i] = rand_weight();

    return m;
}

// Free all memory used by the MLP
void mlp_free(MLP *m) {
    free(m->w_input_hidden);
    free(m->b_hidden);
    free(m->w_hidden_output);
    free(m->b_output);
    free(m);
}

// Forward propagation: compute the output for a given input
void mlp_forward(MLP *m, double *input, double *hidden, double *output) {
    // Compute hidden layer activations
    for (int j = 0; j < m->hidden_size; j++) {
        double sum = m->b_hidden[j]; // Start with bias
        for (int i = 0; i < m->input_size; i++)
            sum += input[i] * m->w_input_hidden[j * m->input_size + i];
        hidden[j] = sigmoid(sum); // Apply activation function
    }

    // Compute output layer activations
    for (int k = 0; k < m->output_size; k++) {
        double sum = m->b_output[k];
        for (int j = 0; j < m->hidden_size; j++)
            sum += hidden[j] * m->w_hidden_output[k * m->hidden_size + j];
        output[k] = sigmoid(sum);
    }
}

// Train the MLP using backpropagation
void mlp_train(MLP *m, double X[][2], double Y[][1], int samples, int epochs, double lr) {
    double hidden[8], output[1]; // Buffers for activations (hidden has max size 8 here)

    for (int e = 0; e < epochs; e++) {
        double total_error = 0.0; // Mean squared error for this epoch

        for (int n = 0; n < samples; n++) {
            // Forward pass
            mlp_forward(m, X[n], hidden, output);

            // Compute output error
            double output_error = Y[n][0] - output[0];
            total_error += output_error * output_error;

            // Compute delta for output neuron
            double delta_out = output_error * sigmoid_derivative(output[0]);

            // Update weights from hidden → output
            for (int j = 0; j < m->hidden_size; j++)
                m->w_hidden_output[j] += lr * delta_out * hidden[j];
            m->b_output[0] += lr * delta_out;

            // Backpropagate error to hidden layer and update input → hidden weights
            for (int j = 0; j < m->hidden_size; j++) {
                double delta_hidden = delta_out * m->w_hidden_output[j] * sigmoid_derivative(hidden[j]);
                for (int i = 0; i < m->input_size; i++)
                    m->w_input_hidden[j * m->input_size + i] += lr * delta_hidden * X[n][i];
                m->b_hidden[j] += lr * delta_hidden;
            }
        }

        // Print error every 10,000 epochs
        if (e % 10000 == 0 || e == epochs - 1)
            printf("Epoch %5d | Error = %.6f\n", e, total_error / samples);
    }
}

// Predict output for new input data
void mlp_predict(MLP *m, double *input, double *output) {
    double hidden[8];
    mlp_forward(m, input, hidden, output);
}

// Save model weights and biases to a binary file
void mlp_save(MLP *m, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }

    // Save layer sizes
    fwrite(&m->input_size, sizeof(int), 1, f);
    fwrite(&m->hidden_size, sizeof(int), 1, f);
    fwrite(&m->output_size, sizeof(int), 1, f);

    // Save weights and biases
    fwrite(m->w_input_hidden, sizeof(double), m->input_size * m->hidden_size, f);
    fwrite(m->b_hidden, sizeof(double), m->hidden_size, f);
    fwrite(m->w_hidden_output, sizeof(double), m->hidden_size * m->output_size, f);
    fwrite(m->b_output, sizeof(double), m->output_size, f);

    fclose(f);
}

// Load a model from a binary file
MLP *mlp_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); exit(1); }

    int in, hid, out;

    // Read layer sizes
    if (fread(&in, sizeof(int), 1, f) != 1 ||
        fread(&hid, sizeof(int), 1, f) != 1 ||
        fread(&out, sizeof(int), 1, f) != 1) {
        fprintf(stderr, "Error reading model header\n");
        fclose(f);
        exit(1);
    }

    // Create MLP with same structure
    MLP *m = mlp_create(in, hid, out);

    // Read all weights and biases
    if (fread(m->w_input_hidden, sizeof(double), in * hid, f) != (size_t)(in * hid) ||
        fread(m->b_hidden, sizeof(double), hid, f) != (size_t)hid ||
        fread(m->w_hidden_output, sizeof(double), hid * out, f) != (size_t)(hid * out) ||
        fread(m->b_output, sizeof(double), out, f) != (size_t)out) {
        fprintf(stderr, "Error reading model data\n");
        fclose(f);
        mlp_free(m);
        exit(1);
    }

    fclose(f);
    return m;
}

int main(int argc, char **argv) {
    srand(time(NULL)); // Initialize random seed
    const char *model_path = "mlp_xor.model";
    MLP *m;

    // If model file exists, load it; otherwise, train a new one
    if (file_exists(model_path)) {
        m = mlp_load(model_path);
    } else {
        // XOR-like dataset (note: inverted XOR pattern)
        double X[4][2] = {{0,0}, {0,1}, {1,0}, {1,1}};
        double Y[4][1] = {{1}, {0}, {0}, {1}};
        m = mlp_create(2, 4, 1);

        // Train the model
        mlp_train(m, X, Y, 4, 100000, 0.1);

        // Save it for next time
        mlp_save(m, model_path);
        mlp_free(m);
        return 0;
    }

    // If two command-line arguments are given, make a prediction
    if (argc == 3) {
        double a = atof(argv[1]), b = atof(argv[2]), output[1];
        double input[2] = {a, b};
        mlp_predict(m, input, output);
        printf("Input: %.0f %.0f -> Output: %.4f ( Round : %.0f)\n", a, b, output[0], round(output[0]));
    }

    mlp_free(m);
    return 0;
}
