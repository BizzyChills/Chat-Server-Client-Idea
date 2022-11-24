
all: ./server/utils.h ./server/utils.c ./server/commands.h ./server/user.h ./server/user.c 
	$(MAKE) test
	$(MAKE) -C ./server
	$(MAKE) -C ./client

CFLAGS=-g

test: test.c ./server/utils.h ./server/utils.c ./server/commands.h ./server/user.h ./server/user.c 
	cc $(CFLAGS) test.c ./server/utils.c ./server/user.c -o t -lm

clean:
	rm -f t
	rm -f ./server/chat-server
	rm -f ./client/c

cleant:
	rm -f t
