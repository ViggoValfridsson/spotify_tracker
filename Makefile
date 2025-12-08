CC = gcc
CFLAGS = -Wall -Wextra -Iinclude $(addprefix -I,$(wildcard lib/*))
DEBUGFLAGS = -g -O0
RELEASEFLAGS = -O2
LDFLAGS = -lm

SRCS = $(wildcard src/*.c)
LIBSRCS = $(wildcard lib/*/*.c)
OBJDIR = obj
BINDIR = bin
OBJS = $(SRCS:src/%.c=$(OBJDIR)/%.o)
LIBOBJS = $(LIBSRCS:lib/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/spotify_tracker

ifeq ($(DEBUG),1)
    CFLAGS += $(DEBUGFLAGS)
else
    CFLAGS += $(RELEASEFLAGS)
endif

all: $(TARGET)

$(TARGET): $(OBJS) $(LIBOBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: lib/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean
