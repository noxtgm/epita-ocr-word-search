# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -Werror
SRC = main.c solver.c
OBJ = $(SRC:.c=.o)
EXEC = $(SRC:.c=)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -fsanitize=address

clean:
	rm -f *.o *.d main $(EXEC)

.PHONY: all clean
