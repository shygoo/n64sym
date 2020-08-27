CC=g++
CFLAGS=-static -I./include -I./build -O3 -s -Wall -Wno-unused-function -Wno-strict-aliasing -Wno-unused-result

LD=g++
LDFLAGS=-s -Wl,--gc-sections,-lm,-lpthread

SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
BUILD_DIR=build

N64SYM=$(BIN_DIR)/n64sym
N64SIG=$(BIN_DIR)/n64sig

BUILTIN_SIGS=$(SRC_DIR)/builtin_signatures.sig
BUILTIN_SIGS_DEFL=$(BUILD_DIR)/builtin_signatures.sig.defl

COMPRESS=tools/bin/compress

.PHONY: all n64sym elf2pj64 elftest clean rebuild_sigs test

all: n64sym n64sig elf2pj64 elftest

########################################

n64sym: $(N64SYM)
n64sig: $(N64SIG)

N64SYM_FILES= \
	n64sym_main \
	n64sym \
	arutil \
	elfutil \
	pathutil \
	crc32 \
	signaturefile \
	threadpool \
	builtin_signatures_include

N64SIG_FILES= \
	n64sig_main \
	n64sig \
	crc32 \
	elfutil \
	arutil \
	pathutil

N64SYM_OBJECTS=$(addprefix $(OBJ_DIR)/,$(addsuffix .o, $(N64SYM_FILES)))
N64SIG_OBJECTS=$(addprefix $(OBJ_DIR)/,$(addsuffix .o, $(N64SIG_FILES)))

$(N64SYM): $(N64SYM_OBJECTS) include/miniz/miniz.c | $(BIN_DIR)
	$(LD) $(N64SYM_OBJECTS) -o $(N64SYM) $(LDFLAGS)

$(N64SIG): $(N64SIG_OBJECTS) | $(BIN_DIR)
	$(LD) $(LDFLAGS) $(N64SIG_OBJECTS) -o $(N64SIG)

########################################

$(BUILTIN_SIGS_DEFL): $(BUILTIN_SIGS) $(COMPRESS) | $(BUILD_DIR)
	$(COMPRESS) $(BUILTIN_SIGS) $(BUILTIN_SIGS_DEFL)

$(COMPRESS):
	make -C tools compress

$(OBJ_DIR)/builtin_signatures_include.o: $(SRC_DIR)/builtin_signatures_include.s $(BUILTIN_SIGS_DEFL)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/builtin_signatures_include.s -o $(OBJ_DIR)/builtin_signatures_include.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $^ -o $@

########################################

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

########################################

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BIN_DIR)
	rm -rf $(BUILD_DIR)
	make -C tools clean

########################################

rebuild_sigs: $(N64SIG)
	$(N64SIG) oslibs > $(BUILTIN_SIGS)

test: $(N64SYM)
	$(N64SYM) test/sm64.bin -s
