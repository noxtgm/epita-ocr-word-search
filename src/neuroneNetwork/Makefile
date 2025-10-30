# === Configuration ===
TARGET = mlp_xor
SRC = neuroneNetwork.c
CC = gcc
CFLAGS = -O2 -Wall -lm
MODEL = mlp_xor.model

# === Règle par défaut ===
all: $(TARGET)

# Compilation
$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

# === Entraîner le modèle (si le modèle n’existe pas encore) ===
train: $(TARGET)
	./$(TARGET)

# === Forcer un nouvel entraînement (supprime l'ancien modèle) ===
retrain: clean_model $(TARGET)
	./$(TARGET)

# === Tester le modèle avec deux valeurs (ex: make test a=1 b=0) ===
test: $(TARGET)
	@if [ ! -f $(MODEL) ]; then \
		echo "⚠️  Modèle introuvable, entraînement en cours..."; \
		./$(TARGET); \
	fi; \
	if [ -z "$(a)" ] || [ -z "$(b)" ]; then \
		echo "❌ Utilisation : make test a=0 b=1"; \
		exit 1; \
	fi; \
	echo "🔹 Test avec entrée : $$a $$b"; \
	./$(TARGET) $$a $$b

# === Nettoyage ===
clean:
	rm -f $(TARGET) $(MODEL)

# Supprimer uniquement le modèle (utile pour retrain)
clean_model:
	rm -f $(MODEL)

