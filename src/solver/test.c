#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "solver.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <grid_file.txt> <word>\n", argv[0]);
        printf("Example: %s test_grid.txt HELLO\n", argv[0]);
        return 1;
    }

    const char *file_name = argv[1];
    const char *word = argv[2];

    // Validate word (must contain only letters)
    for (int i = 0; word[i] != '\0'; i++) {
        if (!isalpha(word[i])) {
            fprintf(stderr, "Error: Word must contain only letters\n");
            return 1;
        }
    }

    // Convert word to uppercase
    size_t word_len = strlen(word);
    char *uppercase_word = malloc(word_len + 1);
    if (uppercase_word == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    for (size_t i = 0; i < word_len; i++) {
        uppercase_word[i] = toupper((unsigned char)word[i]);
    }

    uppercase_word[word_len] = '\0';

    printf("Searching for word '%s' in grid file '%s'\n", uppercase_word, file_name);
    int result = solve_word_search(file_name, uppercase_word);

    free(uppercase_word);
    return result;
}
