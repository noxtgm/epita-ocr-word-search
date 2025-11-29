#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Display ASCII preview of image in terminal for manual labeling
void show_ascii_preview(unsigned char *img, int w, int h);

int main(int argc, char **argv);
