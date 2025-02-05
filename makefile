CC = gcc
INC_DIR = ./include
SRC_DIR = ./source
LIB_DIR = ./lib

CFLAGS = -std=c18 -I$(INC_DIR) \
         -Wall -Wpedantic -Wextra -Wconversion -Wwrite-strings -Wstrict-prototypes -Werror \
         -fstack-protector-all -fpie \
         -pthread -D__USE_XOPEN -D_POSIX_C_SOURCE=200809L -O3

BIN = client demon

vpath %.h  $(INC_DIR)
vpath %.c  $(SRC_DIR)

.PHONY: clean all

all: $(BIN)

client: client.c fonctions_utilitaires.c file_synchronisee.c
	$(CC) $^ -o $@ $(CFLAGS)

demon: demon.c fonctions_utilitaires.c file_synchronisee.c
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -f *~ tube* $(BIN) \#*\# tube_*
