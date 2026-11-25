CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: client

client: main.c registration.h getContent.h pdu.h
	$(CC) $(CFLAGS) -o client main.c

clean:
	rm -f client
