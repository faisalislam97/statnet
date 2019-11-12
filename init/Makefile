all: InitDatabase

CC = gcc
CFLAGS = -Wall
LDFLAGS =-lpthread -ldl

DEPS = $(wildcard *.h)

SRC = $(wildcard *.c)

OBJ = $(patsubst %.c, %.o, $(SRC))


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

InitDatabase: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)
	rm -f *.o

.PHONY: clean

clean:
	rm -f InitDatabase *.o
