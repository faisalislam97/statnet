all: statnet

CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpthread -ldl

DEPS = $(wildcard *.h)

SRC = $(wildcard *.c)

OBJ = $(patsubst %.c, %.o, $(SRC))


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

statnet: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f statnet *.o
