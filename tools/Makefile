CC=g++
CFLAGS=-static -I../include -I./src -O3 -s

SRC_DIR=src
BIN_DIR=bin

COMPRESS=$(BIN_DIR)/compress

$(COMPRESS): $(SRC_DIR)/compress.c ../include/miniz/miniz.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR):
	mkdir $(BIN_DIR)

.PHONY: clean compress

compress: $(COMPRESS)

clean:
	rm -rf $(BIN_DIR)
