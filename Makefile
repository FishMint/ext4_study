CC = gcc
CFLAGS = -Wall -lfuse -D_FILE_OFFSET_BITS=64

SRCDIR = src
OBJDIR = obj
BINDIR = bin
OPS = ops

SRCS := $(wildcard $(SRCDIR)/*.c)
SRCS += $(wildcard $(SRCDIR)/ops/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
EXEC = $(BINDIR)/fuse_ext4

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $^ -o $@ $(CFLAGS) 
    @echo "Executable compiled: $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/*.o $(BINDIR)/* $(OBJDIR)/$(OPS)/*.o
