CC=gcc
CFLAGS=-g -Wall -std=c99
LDFLAGS=
BIN = lili

all: $(BIN)

$(BIN): lili.c
	$(CC) $(CFLAGS) -o $@ $<

debug: lili.c
	$(CC) $(CFLAGS) -DDEBUG -o $(BIN) $<

clean:
	rm -f $(BIN)
