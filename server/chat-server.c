#define _XOPEN_SOURCE 700 // allows use of sigaction
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>
#include <sys/resource.h>
#include <ctype.h>

#include "commands.h"
#include "user.h"
#include "utils.h"

#define BUFFER_SIZE 4096
#define MAX_N_TOKENS 10
#define MAX_CONNECTIONS 1024
#define RANDOM_MSG_LENGTH 8

int writeToFd(int fd, const char *message) {
  int len = strlen(message);
  int totalBytesWritten = 0;

  do {
    int bytesWritten = write(fd, message + totalBytesWritten, len - totalBytesWritten);
    1;
    if( bytesWritten == -1 ) {
      perror("writeToFd|write");
      return -1;
    }

    totalBytesWritten += bytesWritten;
  } while(totalBytesWritten < len);

  return 0;
}

// // HELLO uniqueKey
struct connection *processHello(int argc, char *argv[], struct connection *connList, int fd) {
  struct user *conn;

  char *err = argc !=2 ? "error: expected 'HELLO [key]'\n" : "error: already said HELLO. My pleasantries are exhausted; my apologies. :'(\n";
  
  if( (conn = conn_get(connList, fd)) != NULL ) { // if you have said hello before
    conn->outBuffer = mergeStrings(2, err, "\n");
    return connList;
  }

  if( argc != 2 ) {
    writeToFd(fd, err); // write error to client, can't add to outBuffer because we don't have a connection yet
    return connList;
  }

  char *uniqueKey = argv[1];

  struct user *newConn = conn_create(uniqueKey, fd);

  connList = conn_insert(&connList, newConn);

  newConn->outBuffer = mergeStrings(4, CMD_HELLO, " ", uniqueKey, "\n");

  // writeToFd(newConn->fd, returnMsg); 

  return connList;
}

// GOODBYE
struct user *processGoodbye(int argc, char *argv[], struct user *connList, int fd) {
  struct user *conn = conn_get(connList, fd);
  
  char *err= argc != 1 ? "error: expected 'GOODBYE'\n" : "GOODBYE \"stranger\" (No session created)\n";

  if (conn == NULL) {
    writeToFd(fd, err);
    return connList;
  }
  
  if(argc != 1) {
    conn->outBuffer = mergeStrings(2, err, "\n");
    return connList;
  }
  char *uniqueKey = conn->uname;

  conn->outBuffer = mergeStrings(3, CMD_GOODBYE, ARGS_DELIM_STR, uniqueKey);

  // connList = conn_remove(&connList, conn); // don't remove from list until we've written to the client

  return connList;
}


struct user *processMessage(char *message, struct user *connList, int fd){
  char *tokens[MAX_N_TOKENS] = {"NOOP"};
  int tokenIndex = 0;

  char *messageCopy = malloc(strlen(message) * sizeof(char) + 1);
  messageCopy = strcpy(messageCopy, message);

  char delims[3];
  sprintf(delims, "%c\n", ARGS_DELIM_CHAR);

  char *token = strtok(messageCopy, delims);

  while( token != NULL ) {
    tokens[tokenIndex++] = token;

    token = strtok(NULL, delims);
  }

  char *command = tokens[0];

// //   // TODO: update processHello to handle revised format
  if( strcmp(command, CMD_HELLO) == 0  ) {
    connList = processHello(tokenIndex, tokens, connList, fd);
  }
//   // TODO: update processGoodbye to handle revised format
  else if( strcmp(command, CMD_GOODBYE) == 0 ) {
    connList = processGoodbye(tokenIndex, tokens, connList, fd);
  }
  // TODO: update processSum to handle optional variable
  else if( strcmp(command, CMD_SUM) == 0 ) {
    processSum(tokenIndex, tokens, connList, fd);
  }
  // TODO: handle new STORED_VALUE command
  else if( strcmp(command, CMD_STORED_VALUE) == 0 ) {
    processGet(tokenIndex, tokens, connList, fd);
  }
  
  else if( strcmp(command, CMD_NOOP) == 0 ) {
    // printf("NOOP is technically not an error");
    // writeToFd(fd, "NOOP is technically not an error\n");
    struct user *conn = conn_get(connList, fd);
    
    char *ret = conn == NULL ? "Well, are you going to say HELLO?\n" : "NOOP is technically not an error\n";

    if(conn == NULL) writeToFd(fd, ret);

    else conn->outBuffer = mergeStrings(2, ret, "\0");
  }
  else {
    struct user *conn = conn_get(connList, fd);

    char *ret = conn == NULL ? "error: invalid command. Not even a HELLO huh? :'(\n" : mergeStrings(3, "error: invalid command '", command, "'. Keep your \"arguments\" to yourself\n");
    if(conn == NULL) writeToFd(fd, ret);

    else conn->outBuffer = ret;
  }

  free(messageCopy);

  return connList;
}

struct pollfd createPollFd(int fd, short events) {
    return (struct pollfd) {
        .fd = fd,
        .events = events,
        .revents = 0
    };
}


void usage(int argc, char *argv[]) {
  printf("usage: %s <port-number>\n", argv[0]);
  printf("\tCtrl + C to quit\n\n");

  printf("Supports the following commands:\n");
  printf("\tHELLO uniqueKey\n");
  printf("\t\ton success: sends 'HELLO uniqueKey\\n' through socket\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texample:\n");
  printf("\t\t  HELLO bob\n\n");
  printf("\tSUM [variable-name] number1 [number2 number3 ... numberN]\n");
  printf("\t\ton success: sends '[variable-name] [=] <computedSum>\\n' through socket\n");
  printf("\t\t            if variable-name is present, stores value on a per-client basis.\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texamples:\n");
  printf("\t\t  SUM one 1\n");
  printf("\t\t  SUM one+two 1 2\n");
  printf("\t\t  SUM 9 7 999 -14\n\n");
  printf("\tSTORED-VALUE variable-name\n");
  printf("\t\ton success: sends '[variable-name] [=] <stored-sum>\\n' through socket\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texamples:\n");
  printf("\t\t  STORED-VALUE one\n");
  printf("\t\t  STORED-VALUE one+two\n");
  printf("\tGOODBYE\n");
  printf("\t\tnote: this will close an open connection opened with a HELLO command\n");
  printf("\t\ton success: sends 'GOODBYE uniqueKey\\n' through socket\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texample:\n");
  printf("\t\t  GOODBYE\n");
}

/// very good setup. It looks like everything works!  It looks like you included
///   dynamic-buffer.[ch] as requirements, but they were not required from the build.
///   Be sure to remove these from the Makefile and the header list of sum-server.c
///   -aaron
int main(int argc, char *argv[]){
  if (argc != 2) {
    usage(argc, argv);
    exit(1);
  }
  // setup socket and signal handlers
  // bind socket to port
  struct sockaddr_in address;
  struct in_addr ipAddress;

  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[1]));
  address.sin_addr.s_addr = INADDR_ANY;

  int listenerFd = socket(AF_INET, SOCK_STREAM, 0);

  if( listenerFd == -1 ) {
      perror("socket");
      return -1;
  }

  if( bind(listenerFd, (struct sockaddr *) &address, sizeof(struct sockaddr_in)) == -1 ) {
      perror("bind");
      return -1;
  }

  if(listen(listenerFd, 10) == -1 ) {
      perror("listen");
      return -1;
  }

  sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);

  char buffer[BUFSIZ];
  struct user *clients = NULL;
  struct pollfd clientFds[MAX_CONNECTIONS];
  int numberClients = 0;

  clientFds[numberClients++] = createPollFd(listenerFd, POLLIN);

    for(;;) {
      int numberFdsWithEvents = poll(clientFds, numberClients, -1);

      // check for and add a new connection
      if( clientFds[0].revents & POLLIN ) {
        numberFdsWithEvents -= 1;

        int clientFd = accept(clientFds[0].fd, NULL, NULL);

        if( clientFd == -1 ) {
            perror("accept");
        }

        clientFds[numberClients++] = createPollFd(clientFd, POLLIN);
      }

      int currFd = 1;

      // process events on existing connections
      while(numberFdsWithEvents > 0){
        
        if (clientFds[currFd].revents != 0) {
          numberFdsWithEvents -= 1;

          // check for errors
          if( clientFds[currFd].revents & (POLLHUP | POLLERR) ) {
            struct user *conn = conn_get(clients, clientFds[currFd].fd);
            if (conn != NULL) {
              clients = conn_remove(&clients, conn);
            }
            // close(clientFds[currFd].fd);
            clientFds[currFd] = clientFds[numberClients - 1];
            numberClients -= 1;
            continue;
          }

          // Handle socket data when ready
          if( clientFds[currFd].revents & POLLIN ) {
            int bytesRead = read(clientFds[currFd].fd, buffer, BUFSIZ);

            buffer[bytesRead] = '\0';

            if( bytesRead == -1 ) { // couldn't read
              perror("read");
            }          
            /// thing to watch out for here is to make sure you read all data -aaron
            else if( bytesRead > 0) { // successfully read some data, process command
              char *temp_buf = malloc(BUFSIZ);
              poll(clientFds, numberClients, 0);
              while(clientFds[currFd].revents & POLLIN && read(clientFds[currFd].fd, temp_buf, BUFSIZ) > 0) {
                temp_buf = mergeStrings(2, &buffer, temp_buf);
                poll(clientFds, numberClients, 0);
              }
              clients = processMessage(buffer, clients, clientFds[currFd].fd);
              clientFds[currFd].events |= POLLOUT;
            }
            else{
              struct user *conn = conn_get(clients, clientFds[currFd].fd);
              if (conn != NULL) clients = conn_remove(&clients, conn);
              
              // close(clientFds[currFd].fd);
              clientFds[currFd] = clientFds[numberClients - 1];
              numberClients -= 1;
            }
          }
          if(clientFds[currFd].revents & POLLOUT ) {
            struct user *currClient = conn_get(clients, clientFds[currFd].fd);
            if(currClient == NULL){
              clientFds[currFd].events &= ~POLLOUT;
              continue;
            }
            int bytesWritten = writeToFd(currClient->fd, currClient->outBuffer);
            if (bytesWritten == -1) {
              perror("write");
            }
            while (bytesWritten > 0) {
              bytesWritten = writeToFd(currClient->fd, currClient->outBuffer);
              currClient->outBuffer = &currClient->outBuffer[bytesWritten];
            }
            clientFds[currFd].events = POLLIN;
            currClient->outBuffer = NULL;
          }
        }
        currFd += 1;
      }
    }

    close(clientFds[0].fd);
    free(clientFds);

    return 0;
}
