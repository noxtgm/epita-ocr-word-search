TARGET = mlp_xor
SRC = neuralNetwork.c
CC = gcc
CFLAGS = -O2 -Wall -lm
MODEL = mlp_xor.model

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

train: $(TARGET)
	./$(TARGET)

retrain: $(TARGET)
	rm -f $(MODEL)
	./$(TARGET)

test: $(TARGET)
	@if [ "$(word 2,$(MAKECMDGOALS))" = "" ] || [ "$(word 3,$(MAKECMDGOALS))" = "" ]; then \
		echo "❌ Utilisation : make test <a> <b>"; \
		exit 1; \
	fi; \
	a=$(word 2,$(MAKECMDGOALS)); \
	b=$(word 3,$(MAKECMDGOALS)); \
	echo "🔹 Test avec entrée : $$a $$b"; \
	./$(TARGET) $$a $$b; \
	exit 0

%:
	@:

clean:
	rm -f $(TARGET) $(MODEL)
