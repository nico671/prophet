CC := gcc
CFLAGS := -std=c17 -Wall -Wextra -I src

SRCDIR := src
BUILDDIR := build

# Main program sources (exclude magic_gen.c)
MAIN_SOURCES := $(filter-out $(SRCDIR)/magic_gen.c,$(wildcard $(SRCDIR)/*.c))
MAIN_OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(MAIN_SOURCES))

TARGET := chess
MAGIC_TARGET := magic_gen

.PHONY: all run clean dirs magic

all: dirs $(TARGET)

$(TARGET): $(MAIN_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(MAGIC_TARGET): $(BUILDDIR)/magic_gen.o $(BUILDDIR)/sliding_attacks.o
	$(CC) $(CFLAGS) $^ -o $@

# Compile src/%.c -> build/%.o
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

magic: $(MAGIC_TARGET)
	./$(MAGIC_TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(MAGIC_TARGET)