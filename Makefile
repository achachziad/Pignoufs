# Compilateur et options
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_XOPEN_SOURCE=500 -g -I./include
LDFLAGS = -pthread -lcrypto


# Dossiers
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include

# Création des dossiers principaux
$(shell mkdir -p $(OBJ_DIR))
$(shell mkdir -p $(BIN_DIR))

# Sources et objets
SRCS = $(shell find $(SRC_DIR) -name "*.c")
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Création des sous-dossiers pour les objets
OBJDIRS = $(sort $(dir $(OBJS)))
$(shell mkdir -p $(OBJDIRS))

# Executable
EXEC = $(BIN_DIR)/pignoufs

# Règle principale
all: $(EXEC)

# Création de l'exécutable
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compilation des fichiers sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(EXEC)

# Nettoyage complet
mrproper: clean
	rm -rf $(BIN_DIR)

.PHONY: all clean mrproper

# Règle de debug
debug: CFLAGS += -DDEBUG

debug: all