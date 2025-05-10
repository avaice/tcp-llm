# TCP/IP Client for Lineo uLinux (2002)
# Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -pedantic -std=c89 -D_POSIX_SOURCE -D_GNU_SOURCE -D_BSD_SOURCE
LDFLAGS =

# Target and directories
TARGET = client
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Source and object files
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Default target
all: dirs $(BINDIR)/$(TARGET)

# Create directories if they don't exist
dirs:
	mkdir -p $(OBJDIR) $(BINDIR)

# Link
$(BINDIR)/$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJDIR)/*.o $(BINDIR)/$(TARGET)

# Install
install: $(BINDIR)/$(TARGET)
	install -m 755 $(BINDIR)/$(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build the client (default)"
	@echo "  clean     - Remove generated files"
	@echo "  install   - Install the client to /usr/local/bin"
	@echo "  uninstall - Remove installed client"
	@echo "  help      - Show this message"

.PHONY: all clean install uninstall help dirs
