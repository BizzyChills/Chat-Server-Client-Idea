
all: server
CFLAGS=-g

server: sum-server.c connection.c utils.c commands.h connection.h utils.h
	cc $(CFLAGS) sum-server.c connection.c utils.c -o sum-server

clean:
	rm -f sum-server
