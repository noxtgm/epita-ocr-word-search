#include "solver.h"

// Direction vectors for searching a word in all directions
// Order: right, left, down, up, down-right, down-left, up-right, up-left
static const int dx[] = {0, 0, 1, -1, 1, -1, 1, -1};
static const int dy[] = {1, -1, 0, 0, 1, 1, -1, -1};

// Load the grid from a file
int load_grid(const char *file_name, Grid *grid)
{
    if (file_name == NULL || grid == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 0;
    }

    FILE *file = fopen(file_name, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", file_name);
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    grid->rows = 0;
    grid->cols = 0;

    // Read file line by line
    while (fgets(line, sizeof(line), file) && grid->rows < MAX_GRID_SIZE) {
        // Remove the newline character
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        // Determine the number of columns (first line only)
        if (grid->rows == 0) {
            if (len == 0) {
                fprintf(stderr, "Error: First line cannot be empty\n");
                fclose(file);
                return 0;
            }
            grid->cols = len;
        }
        
        // Check that all lines have the same length
        if (len != grid->cols) {
            fprintf(stderr, "Error: All lines must have the same length\n");
            fclose(file);
            return 0;
        }

        // Copy the line into the grid
        for (int j = 0; j < len; j++) {
            grid->grid[grid->rows][j] = line[j];
        }
        
        grid->rows++;
    }

    fclose(file);
    
    // Check the minimum dimensions
    if (grid->rows < 5 || grid->cols < 5) {
        fprintf(stderr, "Error: Grid must be at least 5x5\n");
        return 0;
    }

    return 1;
}

// Check if a position is valid in the grid
int is_valid_position(const Grid *grid, int x, int y)
{
    if (grid == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 0;
    }

    return x >= 0 && x < grid->rows && y >= 0 && y < grid->cols;
}

// Search for a word in a specific direction from a starting position
int search_word_in_direction(const Grid *grid, const char *word, int start_x,
                             int start_y, int dir, Position *end_pos)
{
    if (grid == NULL || word == NULL || end_pos == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 0;
    }

    int word_len = strlen(word);
    int x = start_x;
    int y = start_y;

    // Check each character of the word
    for (int i = 0; i < word_len; i++) {
        // Check if the position is valid
        if (!is_valid_position(grid, x, y)) {
            return 0;
        }

        // Check if the character matches
        if (grid->grid[x][y] != word[i]) {
            return 0;
        }

        // Save the end position if it's the last character
        if (i == word_len - 1) {
            end_pos->x = x;
            end_pos->y = y;
        }

        // Go in the specified direction
        x += dx[dir];
        y += dy[dir];
    }

    return 1;
}

// Search for a word in the entire grid
int search_word(const Grid *grid, const char *word, Position *start_pos,
                Position *end_pos)
{
    if (grid == NULL || word == NULL || start_pos == NULL || end_pos == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 0;
    }

    // Iterate through each position in the grid
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            // If the first character matches
            if (grid->grid[i][j] == word[0]) {
                // Try all directions
                for (int dir = 0; dir < 8; dir++) {
                    if (search_word_in_direction(grid, word, i, j, dir, end_pos)) {
                        start_pos->x = i;
                        start_pos->y = j;
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

// Solve the word search
int solve_word_search(const char *file_name, const char *word)
{
    if (file_name == NULL || word == NULL) {
        fprintf(stderr, "Error: Invalid parameters\n");
        return 1;
    }

    if (word[0] == '\0') {
        fprintf(stderr, "Error: Word cannot be empty\n");
        return 1;
    }

    Grid grid;
    Position start_pos, end_pos;

    // Load the grid
    if (!load_grid(file_name, &grid)) {
        fprintf(stderr, "Error: Failed to load grid\n");
        return 1;
    }

    // Search for the word
    if (search_word(&grid, word, &start_pos, &end_pos)) {
        fprintf(stderr, "(%d,%d)(%d,%d)\n", start_pos.y, start_pos.x, end_pos.y, end_pos.x);
        return 0;
    } else {
        fprintf(stderr, "Not found\n");
        return 2;
    }
}

// Main solver function which tests for edge cases and runs the solver
// Returns: 0 = success, 1 = error, 2 = word not found
int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <grid_file> <word>\n", argv[0]);
        return 1;
    }

    const char *file_name = argv[1];
    const char *word = argv[2];

    // Validate word is not empty
    if (word[0] == '\0') {
        fprintf(stderr, "Error: Word cannot be empty\n");
        return 1;
    }

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

    int result = solve_word_search(file_name, uppercase_word);

    free(uppercase_word);
    return result;
}
