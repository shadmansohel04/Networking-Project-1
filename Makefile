CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean server client

all: server client

server:
	$(CC) $(CFLAGS) -o server/server server/main.c

client:
	$(CC) $(CFLAGS) -o client/client client/main.c

clean:
	rm -f server/server client/client
