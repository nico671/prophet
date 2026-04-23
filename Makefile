CC ?= cc

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

BUILD ?= dev

CSTD := -std=c17
WARNFLAGS := -Wall -Wextra
CPPFLAGS := -I src

# Apple Silicon tuning for local machine performance.
ARCHFLAGS :=
ifeq ($(UNAME_S),Darwin)
ifeq ($(UNAME_M),arm64)
ARCHFLAGS += -mcpu=native
endif
endif

DEBUG_CFLAGS := -O0 -g3 -fno-omit-frame-pointer
DEV_CFLAGS := -O3 -g1 -DNDEBUG
RELEASE_CFLAGS := -O3 -DNDEBUG -flto

DEBUG_LDFLAGS :=
DEV_LDFLAGS :=
RELEASE_LDFLAGS :=

ifeq ($(BUILD),debug)
MODE_CFLAGS := $(DEBUG_CFLAGS)
MODE_LDFLAGS := $(DEBUG_LDFLAGS)
else ifeq ($(BUILD),release)
MODE_CFLAGS := $(RELEASE_CFLAGS)
MODE_LDFLAGS := $(RELEASE_LDFLAGS)
else
MODE_CFLAGS := $(DEV_CFLAGS)
MODE_LDFLAGS := $(DEV_LDFLAGS)
endif

CFLAGS := $(CSTD) $(WARNFLAGS) $(ARCHFLAGS) $(MODE_CFLAGS)
LDFLAGS := $(MODE_LDFLAGS)

SRCDIR := src
BUILDDIR := build

# Recursively find all .c files except magic_gen.c and perft tests
MAIN_SOURCES := $(shell find $(SRCDIR) -name '*.c' ! -name 'magic_gen.c' ! -name 'perft_test.c')
MAIN_OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(MAIN_SOURCES))

TARGET := prophet
MAGIC_TARGET := magic_gen
PERFT_TARGET := perft

.PHONY: all run clean dirs magic perft_test debug dev release

all: dirs $(TARGET)

$(TARGET): $(MAIN_OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(MAGIC_TARGET): $(BUILDDIR)/attacks/magic_gen.o $(BUILDDIR)/attacks/sliding_attacks.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(PERFT_TARGET): $(BUILDDIR)/tests/perft_test.o $(BUILDDIR)/tests/testing_utils.o $(BUILDDIR)/movegen/move.o $(BUILDDIR)/eval/hceval.o $(BUILDDIR)/search/tt.o $(BUILDDIR)/engine/engine.o $(BUILDDIR)/movegen/movegen.o $(BUILDDIR)/movegen/pawn_moves.o $(BUILDDIR)/movegen/knight_moves.o $(BUILDDIR)/movegen/sliding_moves.o $(BUILDDIR)/movegen/king_moves.o $(BUILDDIR)/movegen/move_make.o $(BUILDDIR)/board/cboard.o $(BUILDDIR)/board/zobrist.o $(BUILDDIR)/engine/engine.o $(BUILDDIR)/utils/prng.o $(BUILDDIR)/attacks/constant_attacks.o $(BUILDDIR)/attacks/sliding_attacks.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

magic: $(MAGIC_TARGET)
	./$(MAGIC_TARGET)

perft_test: $(PERFT_TARGET)
	./$(PERFT_TARGET)

debug:
	$(MAKE) clean all BUILD=debug

dev:
	$(MAKE) clean all BUILD=dev

release:
	$(MAKE) clean all BUILD=release

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(MAGIC_TARGET) $(PERFT_TARGET)

dirs:
	@mkdir -p $(BUILDDIR)