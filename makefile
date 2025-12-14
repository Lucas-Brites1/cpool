CC = gcc
CFLAGS = -Wall -Iinclude -pthread

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

SRC = src/cpool.c
OBJ = src/cpool.o
TARGET_LIB = libcpool$(LIB_EXT)

EXAMPLE_SRC = examples/simple.c
EXAMPLE_BIN = example$(EXEC_EXT)

all: $(TARGET_LIB)

$(TARGET_LIB): $(OBJ)
	ar rcs $@ $^

example: $(TARGET_LIB)
	$(CC) $(EXAMPLE_SRC) -o $(EXAMPLE_BIN) -Iinclude -L. -lcpool -pthread

run: example
	.$(call FixPath,/$(EXAMPLE_BIN))

clean:
	$(RM) $(call FixPath,src/*.o) $(TARGET_LIB) $(EXAMPLE_BIN)
