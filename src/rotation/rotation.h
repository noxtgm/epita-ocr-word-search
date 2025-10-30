#ifndef ROTATION_H
#define ROTATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define PI 3.14159265358979323846 // Pi constant for angle calculations

// Rotate an image by a given angle in degrees
GdkPixbuf* rotate_image(GdkPixbuf *pixbuf, double angle);

// Load an image from a file
GdkPixbuf* load_image(const char *filename);

// Save an image to a file
int save_image(GdkPixbuf *pixbuf, const char *filename);

#endif
