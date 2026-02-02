CC := gcc
CFLAGS := -std=c17 -Wall -Wextra -I src

SRCDIR := src
BUILDDIR := build

# Recursively find all .c files except magic_gen.c and perft tests
MAIN_SOURCES := $(shell find $(SRCDIR) -name '*.c' ! -name 'magic_gen.c' ! -name 'perft_test.c')
MAIN_OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(MAIN_SOURCES))

TARGET := chess
MAGIC_TARGET := magic_gen
PERFT_TARGET := perft

.PHONY: all run clean dirs magic perft_test

all: dirs $(TARGET)

$(TARGET): $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(MAGIC_TARGET): $(BUILDDIR)/attacks/magic_gen.o $(BUILDDIR)/attacks/sliding_attacks.o
	$(CC) $(CFLAGS) $^ -o $@

$(PERFT_TARGET): $(BUILDDIR)/tests/perft_test.o $(BUILDDIR)/tests/testing_utils.o $(BUILDDIR)/movegen/movegen.o $(BUILDDIR)/movegen/pawn_moves.o $(BUILDDIR)/movegen/knight_moves.o $(BUILDDIR)/movegen/sliding_moves.o $(BUILDDIR)/movegen/king_moves.o $(BUILDDIR)/movegen/move_make.o $(BUILDDIR)/board/cboard.o $(BUILDDIR)/board/zobrist.o $(BUILDDIR)/engine/engine.o $(BUILDDIR)/utils/prng.o $(BUILDDIR)/attacks/constant_attacks.o $(BUILDDIR)/attacks/sliding_attacks.o
	$(CC) $(CFLAGS) $^ -o $@

# Compile src/%.c -> build/%.o (this already works correctly)
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

magic: $(MAGIC_TARGET)
	./$(MAGIC_TARGET)

perft_test: $(PERFT_TARGET)
	./$(PERFT_TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(MAGIC_TARGET) $(PERFT_TARGET)

dirs:
	@mkdir -p $(BUILDDIR)