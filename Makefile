# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Linker flags
LDFLAGS = -lX11

# Target executable
TARGET = xfacs

# Source file
SRC = xfacs.c

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean
