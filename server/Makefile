CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: server

server: main.c registry.h pdu.h
	$(CC) $(CFLAGS) -o server main.c

clean:
	rm -f server
