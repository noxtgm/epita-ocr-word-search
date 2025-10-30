#ifndef DETECT_LIST_H
#define DETECT_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <gtk/gtk.h>

typedef struct {
    int x, y;
} Point;

typedef struct {
    int minx, maxx, miny, maxy;
    int pixel_count;
} Box;

#endif