# ============================================================================
# tarsau - Sistem Programlama Arsiv Projesi Makefile
# Linux/Unix: make && make test
# ============================================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -Wpedantic -std=c11 -O2 -Iinclude
CFLAGS  += $(if $(filter Windows_NT,$(OS)),,-D_POSIX_C_SOURCE=200809L)
LDFLAGS  =
TARGET   = bin/tarsau

SRC_DIR  = src
INC_DIR  = include
OBJ_DIR  = obj
BIN_DIR  = bin

SOURCES  = $(SRC_DIR)/main.c \
           $(SRC_DIR)/args.c \
           $(SRC_DIR)/common.c \
           $(SRC_DIR)/validate.c \
           $(SRC_DIR)/archive.c \
           $(SRC_DIR)/extract.c \
           $(SRC_DIR)/tarsau_format.c

OBJECTS  = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Tek dosya derleme: gcc -Wall -std=c11 -D_POSIX_C_SOURCE=200809L -o tarsau tarsau.c
SINGLE   = tarsau.c
SINGLE_BIN = tarsau

.PHONY: all clean test dirs single

all: dirs $(TARGET)

single:
	$(CC) $(CFLAGS) -o $(SINGLE_BIN) $(SINGLE)
	@echo "Tek dosya derleme: ./$(SINGLE_BIN)"

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Derleme tamamlandi: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f tests/out/*.sau
	@echo "Temizlik tamamlandi."

test: all
	@chmod +x tests/run_tests.sh 2>/dev/null || true
	@bash tests/run_tests.sh
