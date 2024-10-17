# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -O2

# Target executable name
TARGET = gol.out

# Source files
SRCS = gol.c

# Object files
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET)
