CC=gcc
CFLAGS=-g -Wall -std=c99
LDFLAGS=

SRC=lili.c
OBJ=$(SRC:.c=.o)

all: lili

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

lili: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f lili $(OBJ)
