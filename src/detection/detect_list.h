#ifndef DETECT_LIST_H
#define DETECT_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>

// Platform-specific mkdir declarations
#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path, mode) mkdir(path, mode)
#endif

#ifndef POINT_TYPE_DEFINED
typedef struct {
    int x, y;
} Point;
#endif

typedef struct {
    int minx, maxx, miny, maxy;
    int pixel_count;
} Box;

#endif