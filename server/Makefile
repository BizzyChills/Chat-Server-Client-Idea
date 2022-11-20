
all: server
CFLAGS=-g

server: chat-server.c user.c utils.c commands.h user.h utils.h
	cc $(CFLAGS) chat-server.c user.c utils.c -o chat-server

clean:
	rm -f chat-server
