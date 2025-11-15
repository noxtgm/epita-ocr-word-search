#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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


// Check if a file exists 
int file_exists(const char *path) 
{
  FILE *f = fopen(path, "rb");
  if (f) 
  { 
    fclose(f);
    return 1;
  } 
  return 0; 
}

// -----------------------------
//  Create & Free
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

// -----------------------------
//  Forward Pass
// -----------------------------

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

// -----------------------------
//  Training (Cross-Entropy + Softmax)
// -----------------------------

void mlp_train(MLP *m, double **X, double **Y, int samples, int epochs, double lr) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    double *output = malloc(sizeof(double) * m->output_size);

    for (int e = 0; e < epochs; e++) {
        double total_error = 0.0;

        for (int n = 0; n < samples; n++) {
            mlp_forward(m, X[n], hidden, output);

            double delta_out[m->output_size];

            for (int k = 0; k < m->output_size; k++) {
                delta_out[k] = output[k] - Y[n][k];
                total_error += -Y[n][k] * log(output[k] + 1e-12);
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
                    m->w_input_hidden[j * m->input_size + i] -= lr * delta_hidden[j] * X[n][i];
                m->b_hidden[j] -= lr * delta_hidden[j];
            }
        }

        if (e % 50 == 0)
            printf("Epoch %5d | Error = %.6f\n", e, total_error / samples);
    }

    free(hidden);
    free(output);
}

// -----------------------------
//  Predict
// -----------------------------

void mlp_predict(MLP *m, double *input, double *output) {
    double *hidden = malloc(sizeof(double) * m->hidden_size);
    mlp_forward(m, input, hidden, output);
    free(hidden);
}

// -----------------------------
//  Save / Load
// -----------------------------

MLP *mlp_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); exit(1); }

    int in, hid, out;
    fread(&in, sizeof(int), 1, f);
    fread(&hid, sizeof(int), 1, f);
    fread(&out, sizeof(int), 1, f);

    MLP *m = mlp_create(in, hid, out);

    fread(m->w_input_hidden, sizeof(double), in * hid, f);
    fread(m->b_hidden, sizeof(double), hid, f);
    fread(m->w_hidden_output, sizeof(double), hid * out, f);
    fread(m->b_output, sizeof(double), out, f);

    fclose(f);
    return m;
}

void mlp_save(MLP *m, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }

    fwrite(&m->input_size, sizeof(int), 1, f);
    fwrite(&m->hidden_size, sizeof(int), 1, f);
    fwrite(&m->output_size, sizeof(int), 1, f);

    fwrite(m->w_input_hidden, sizeof(double), m->input_size * m->hidden_size, f);
    fwrite(m->b_hidden, sizeof(double), m->hidden_size, f);
    fwrite(m->w_hidden_output, sizeof(double), m->hidden_size * m->output_size, f);
    fwrite(m->b_output, sizeof(double), m->output_size, f);

    fclose(f);
}

