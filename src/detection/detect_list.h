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

// Character information structure
typedef struct {
    int char_index;
    int minx, maxx, miny, maxy;
    int width, height;
    int pixel_count;
    int center_x, center_y;
} CharInfo;

// Word information structure
typedef struct {
    int word_index;
    int minx, maxx, miny, maxy;  // Word bounding box
    int width, height;
    int char_count;
    int total_pixels;
    int center_x, center_y;
    CharInfo* chars;  // Array of characters in this word
} WordInfo;

// Character group structure (for spacing-based grouping)
typedef struct {
    int group_index;
    int char_count;
    int* char_indices;  // Indices into the original chars array
    float avg_spacing;  // Average spacing between characters in this group
} CharGroup;

#endif