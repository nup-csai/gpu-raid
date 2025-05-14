CC       := gcc
CFLAGS   := -O3 -march=native -std=gnu11 -Wall -Wextra -pedantic
LDFLAGS  := -lrt

SRCDIR   := src
BINDIR   := bin

SRCS   := $(wildcard $(SRCDIR)/*.c)
PROGS  := $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%,$(SRCS))

.PHONY: all clean

all: $(PROGS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/%: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BINDIR)
