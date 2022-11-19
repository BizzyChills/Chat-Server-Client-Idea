
all: server
CFLAGS=-g

server: sum-server.c user.c utils.c commands.h user.h utils.h
	cc $(CFLAGS) sum-server.c user.c utils.c -o sum-server

test: test.c
	cc $(CFLAGS) test.c -o t
clean:
	rm -f sum-server
