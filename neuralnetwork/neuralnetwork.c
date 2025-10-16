#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

double rand_double() { return (2.0 * rand() / RAND_MAX) - 1.0; } // [-1,1]

// Sigmoid activation and derivative
double sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }
double dsigmoid(double y) { return y * (1.0 - y); } // y = sigmoid(x)

// Network structure (input, hidden, output)
typedef struct {
    int in_size;
    int hidden_size;
    int out_size;

    // Matrices stored in 1D (row-major)
    double *w1; // weights input->hidden (hidden_size x in_size)
    double *b1; // hidden biases (hidden_size)
    double *w2; // weights hidden->out (out_size x hidden_size)
    double *b2; // output biases (out_size)

    // Buffers for forward pass
    double *hidden; // hidden layer values
    double *output; // output values
} MLP;


// Prototypes
MLP* mlp_create(int in_size, int hidden_size, int out_size);
void mlp_free(MLP *m);
void mlp_forward(MLP *m, const double *input);
void mlp_predict(MLP *m, const double *x, double *out_buf);
void mlp_train(MLP *m, const double *X, const double *Y, int n_samples, int epochs, double lr);
void mlp_save(MLP *m, const char *filename);
MLP* mlp_load(const char *filename);
int file_exists(const char *filename);

// Save in binary format
void mlp_save(MLP *m, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); exit(1); }

    // Write dimensions
    fwrite(&m->in_size, sizeof(int), 1, f);
    fwrite(&m->hidden_size, sizeof(int), 1, f);
    fwrite(&m->out_size, sizeof(int), 1, f);

    // Write weights and biases
    fwrite(m->w1, sizeof(double), m->hidden_size * m->in_size, f);
    fwrite(m->b1, sizeof(double), m->hidden_size, f);
    fwrite(m->w2, sizeof(double), m->out_size * m->hidden_size, f);
    fwrite(m->b2, sizeof(double), m->out_size, f);

    fclose(f);
    printf("Model saved to %s\n", filename);
}

// Load model
MLP* mlp_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("fopen"); exit(1); }

    int in_size, hidden_size, out_size;
    fread(&in_size, sizeof(int), 1, f);
    fread(&hidden_size, sizeof(int), 1, f);
    fread(&out_size, sizeof(int), 1, f);

    MLP *m = mlp_create(in_size, hidden_size, out_size);

    fread(m->w1, sizeof(double), hidden_size * in_size, f);
    fread(m->b1, sizeof(double), hidden_size, f);
    fread(m->w2, sizeof(double), out_size * hidden_size, f);
    fread(m->b2, sizeof(double), out_size, f);

    fclose(f);
    printf("Model loaded from %s\n", filename);
    return m;
}

// Allocation and initialization
MLP* mlp_create(int in_size, int hidden_size, int out_size) {
    MLP *m = malloc(sizeof(MLP));
    if (!m) { perror("malloc MLP"); exit(1); }
    
    m->in_size = in_size;
    m->hidden_size = hidden_size;
    m->out_size = out_size;

    m->w1 = malloc(sizeof(double) * hidden_size * in_size);
    m->b1 = malloc(sizeof(double) * hidden_size);
    m->w2 = malloc(sizeof(double) * out_size * hidden_size);
    m->b2 = malloc(sizeof(double) * out_size);
    m->hidden = malloc(sizeof(double) * hidden_size);
    m->output = malloc(sizeof(double) * out_size);

    if (!m->w1 || !m->b1 || !m->w2 || !m->b2 || !m->hidden || !m->output) {
        perror("malloc weights/buffers");
        exit(1);
    }

    // Random initialization
    for (int i = 0; i < hidden_size * in_size; ++i) m->w1[i] = rand_double() * 0.5; // small amplitude
    for (int i = 0; i < hidden_size; ++i) m->b1[i] = 0.0;
    for (int i = 0; i < out_size * hidden_size; ++i) m->w2[i] = rand_double() * 0.5;
    for (int i = 0; i < out_size; ++i) m->b2[i] = 0.0;

    return m;
}

void mlp_free(MLP *m) {
    free(m->w1); free(m->b1); free(m->w2); free(m->b2);
    free(m->hidden); free(m->output);
    free(m);
}

// Simple matrix-vector product for dense layer
// y = W * x + b  (W dim: rows x cols)
void dense_forward(const double *W, const double *b, int rows, int cols, const double *x, double *y) {
    for (int i = 0; i < rows; ++i) {
        double s = b ? b[i] : 0.0;
        const double *Wrow = W + i * cols;
        for (int j = 0; j < cols; ++j) s += Wrow[j] * x[j];
        y[i] = s;
    }
}

// Forward pass
void mlp_forward(MLP *m, const double *input) {
    // hidden = sigmoid(W1 * input + b1)
    dense_forward(m->w1, m->b1, m->hidden_size, m->in_size, input, m->hidden);
    for (int i = 0; i < m->hidden_size; ++i) m->hidden[i] = sigmoid(m->hidden[i]);

    // output = sigmoid(W2 * hidden + b2)
    dense_forward(m->w2, m->b2, m->out_size, m->hidden_size, m->hidden, m->output);
    for (int i = 0; i < m->out_size; ++i) m->output[i] = sigmoid(m->output[i]);
}

// Training with gradient descent (batch)
// X : array of examples (n_samples x in_size) row-major
// Y : labels (n_samples x out_size) row-major
void mlp_train(MLP *m, const double *X, const double *Y, int n_samples, int epochs, double lr) {
    // Buffers for gradients
    double *dW2 = malloc(sizeof(double) * m->out_size * m->hidden_size);
    double *db2 = malloc(sizeof(double) * m->out_size);
    double *dW1 = malloc(sizeof(double) * m->hidden_size * m->in_size);
    double *db1 = malloc(sizeof(double) * m->hidden_size);
    
    // Allocate delta buffers once outside the loop
    double *delta_out = malloc(sizeof(double) * m->out_size);
    double *delta_hidden = malloc(sizeof(double) * m->hidden_size);

    if (!dW2 || !db2 || !dW1 || !db1 || !delta_out || !delta_hidden) {
        perror("malloc gradients");
        exit(1);
    }

    for (int e = 0; e < epochs; ++e) {
        // Zero out the gradients (batch)
        for (int i = 0; i < m->out_size * m->hidden_size; ++i) dW2[i] = 0.0;
        for (int i = 0; i < m->out_size; ++i) db2[i] = 0.0;
        for (int i = 0; i < m->hidden_size * m->in_size; ++i) dW1[i] = 0.0;
        for (int i = 0; i < m->hidden_size; ++i) db1[i] = 0.0;

        double loss = 0.0;

        // For each example (batch gradient)
        for (int s = 0; s < n_samples; ++s) {
            const double *x = X + s * m->in_size;
            const double *y_true = Y + s * m->out_size;

            // Forward (recompute activations for each sample)
            mlp_forward(m, x);

            // Compute loss (MSE) and delta for output
            for (int i = 0; i < m->out_size; ++i) {
                double err = m->output[i] - y_true[i];
                loss += 0.5 * err * err;
                delta_out[i] = err * dsigmoid(m->output[i]); // dL/dz_out
                db2[i] += delta_out[i];
            }

            // grad W2 += delta_out * hidden^T
            for (int i = 0; i < m->out_size; ++i) {
                double *W2row = dW2 + i * m->hidden_size;
                for (int j = 0; j < m->hidden_size; ++j)
                    W2row[j] += delta_out[i] * m->hidden[j];
            }

            // Backprop to hidden: delta_hidden = (W2^T * delta_out) * dsigmoid(hidden)
            for (int j = 0; j < m->hidden_size; ++j) {
                double ssum = 0.0;
                for (int i = 0; i < m->out_size; ++i)
                    ssum += m->w2[i * m->hidden_size + j] * delta_out[i];
                delta_hidden[j] = ssum * dsigmoid(m->hidden[j]);
                db1[j] += delta_hidden[j];
            }

            // grad W1 += delta_hidden * x^T
            for (int j = 0; j < m->hidden_size; ++j) {
                double *W1row = dW1 + j * m->in_size;
                for (int k = 0; k < m->in_size; ++k)
                    W1row[k] += delta_hidden[j] * x[k];
            }
        } // End samples

        // Apply gradients (gradient descent, average over batch)
        double inv_n = 1.0 / n_samples;
        for (int i = 0; i < m->out_size * m->hidden_size; ++i) m->w2[i] -= lr * (dW2[i] * inv_n);
        for (int i = 0; i < m->out_size; ++i) m->b2[i] -= lr * (db2[i] * inv_n);
        for (int i = 0; i < m->hidden_size * m->in_size; ++i) m->w1[i] -= lr * (dW1[i] * inv_n);
        for (int i = 0; i < m->hidden_size; ++i) m->b1[i] -= lr * (db1[i] * inv_n);

        if ((e % 1000) == 0 || e == epochs - 1) {
            printf("Epoch %d/%d, loss=%.6f\n", e, epochs, loss * inv_n);
        }
    } // End epochs

    // Free delta buffers
    free(delta_out);
    free(delta_hidden);
    free(dW2); free(db2); free(dW1); free(db1);
}

// Prediction (forward + return output vector)
void mlp_predict(MLP *m, const double *x, double *out_buf) {
    mlp_forward(m, x);
    for (int i = 0; i < m->out_size; ++i) out_buf[i] = m->output[i];
}

int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

int main() {
    srand((unsigned) time(NULL));

    double X[4][2] = { {0,0}, {0,1}, {1,0}, {1,1} };
    double Y[4][1] = { {1}, {0}, {0}, {1} };

    double Xbuf[4*2], Ybuf[4*1];
    for (int i=0;i<4;i++) for (int j=0;j<2;j++) Xbuf[i*2 + j] = X[i][j];
    for (int i=0;i<4;i++) Ybuf[i] = Y[i][0];

    const char *model_file = "mlp_xor.model";
    MLP *m;

    if (file_exists(model_file)) {
        m = mlp_load(model_file);
    } else {
        m = mlp_create(2, 4, 1);
        mlp_train(m, Xbuf, Ybuf, 4, 100000, 0.1);
        mlp_save(m, model_file);
    }

    double out[1];
    printf("\nModel results:\n");
    for (int i=0;i<4;i++) {
        mlp_predict(m, Xbuf + i*2, out);
        printf("in: %.0f %.0f -> out: %.4f (expected %.0f)\n", 
               Xbuf[i*2], Xbuf[i*2+1], out[0], Ybuf[i]);
    }

    mlp_free(m);
    return 0;
}
