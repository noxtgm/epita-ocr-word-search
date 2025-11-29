#include "neuralnetwork.h"

// Enhanced image preprocessing with adaptive thresholding for better font handling
double* load_image_enhanced(const char *path);

// Data augmentation structure: stores augmented version + its label
typedef struct {
    double *data;
    int label;
} AugmentedSample;

// Generate multiple variations of an image (noise, shifts) for data augmentation
AugmentedSample* augment_image(double *img, int label, int *count);

// Load dataset and apply data augmentation to each image
Dataset* load_dataset_with_augmentation(const char *list_file);

// Enhanced training with dropout regularization, L2 decay, and early stopping
void mlp_train_enhanced(MLP *m, Dataset *train, Dataset *val, int epochs, double lr, const char *save_path);

int main(int argc, char **argv);
