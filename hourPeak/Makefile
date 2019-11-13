all: hourPeak

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread -ldl

DEPS = $(wildcard *.h)

SRC = $(wildcard *.c)

OBJ = $(patsubst %.c, %.o, $(SRC))


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hourPeak: $(OBJ)
	$(CC) -g -o $@ $^ $(LDFLAGS)
	rm *.o
.PHONY: clean

clean:
	rm -f hourPeak *.o
