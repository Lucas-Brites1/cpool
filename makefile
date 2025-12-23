CC = gcc
CFLAGS = -Wall -Iinclude -pthread -g -O2

ifdef OS
   RM = del /Q
   FixPath = $(subst /,\,$1)
   EXEC_EXT = .exe
   LIB_EXT = .a
else
   RM = rm -f
   FixPath = $1
   EXEC_EXT =
   LIB_EXT = .a
endif

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

TARGET_LIB = libcpool$(LIB_EXT)

EXAMPLE_SRC = examples/example1.c
EXAMPLE_BIN = test_pool$(EXEC_EXT)

.PHONY: all clean run example

all: $(TARGET_LIB) example
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(OBJ)
	ar rcs $@ $^

example: $(TARGET_LIB)
	$(CC) $(EXAMPLE_SRC) -o $(EXAMPLE_BIN) -Iinclude -L. -lcpool -pthread

run: example
	./$(EXAMPLE_BIN)

clean:
	$(RM) $(call FixPath,src/*.o) $(TARGET_LIB) $(EXAMPLE_BIN)
