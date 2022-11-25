
all: ./server/chat-server.c ./server/utils.h ./server/utils.c ./server/commands.h ./server/user.h ./server/user.c 
	$(MAKE) -C ./server
	$(MAKE) -C ./client

CFLAGS=-g

server: ./server/chat-server.c ./server/utils.h ./server/utils.c ./server/commands.h ./server/user.h ./server/user.c 
	$(MAKE) -C ./server

client: ./client/chat-client.c
	$(MAKE) -C ./client

clean:
	rm -f ./server/chat-server
	rm -f ./client/chat-client

