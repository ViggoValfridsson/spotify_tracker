CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
DEBUGFLAGS = -g -O0
RELEASEFLAGS = -O2

SRC = $(wildcard src/*.c)
OBJDIR = obj
BINDIR = bin
OBJ = $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRC))
TARGET = $(BINDIR)/spotify_tracker

ifeq ($(DEBUG),1)
    CFLAGS += $(DEBUGFLAGS)
else
    CFLAGS += $(RELEASEFLAGS)
endif

all: $(TARGET)

$(TARGET): $(OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
