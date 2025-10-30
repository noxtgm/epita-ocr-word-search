# Makefile
CC = gcc
CFLAGS = -Wall -Wextra
EXEC = solver

# Source files
SRC = test.c solver.c
OBJ = $(SRC:.c=.o)

# Default target
all: test

# Build executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Build and run tests
test: $(EXEC)
	@echo "=== Testing Word Search Solver ==="
	@if [ -f "test_grid.txt" ]; then \
		echo "Running tests with test_grid.txt:"; \
		echo "Test 1: Searching for HELLO"; \
		./$(EXEC) test_grid.txt HELLO; \
		echo ""; \
		echo "Test 2: Searching for AGM"; \
		./$(EXEC) test_grid.txt AGM; \
		echo ""; \
		echo "Test 3: Searching for nonexistent word"; \
		./$(EXEC) test_grid.txt XYZ; \
	else \
		echo "No test_grid.txt found. Creating a sample test grid..."; \
		echo "HELLO" > test_grid.txt; \
		echo "WORLD" >> test_grid.txt; \
		echo "OCAML" >> test_grid.txt; \
		echo "EPITA" >> test_grid.txt; \
		echo "SOLVE" >> test_grid.txt; \
		echo "Sample test_grid.txt created. Running tests:"; \
		echo "Test 1: Searching for HELLO"; \
		./$(EXEC) test_grid.txt HELLO; \
		echo ""; \
		echo "Test 2: Searching for WORLD"; \
		./$(EXEC) test_grid.txt WORLD; \
	fi

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f *.o $(EXEC)

# Clean everything including test files
distclean: clean
	rm -f test_grid.txt

.PHONY: all test clean distclean
