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
#define TEMP_PORT 57300

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
struct user *processHello(int argc, char *argv[], struct user *connList, int fd) {
  struct user *conn;
  char *username = argv[1];

  if( (conn = get_user(connList, fd, argv[1])) != NULL ) { // if you have said hello before
    return connList;
  }

  if( argc != 2 ) {
    char *err = "error: expected 'HELLO [uname]'\n";
    writeToFd(fd, err); // write error to client, can't add to outbuffer because we don't have a connection yet
    return connList;
  }

  struct user *newConn = conn_create(username, fd);

  connList = conn_insert(&connList, newConn);

  return connList;
}

// GOODBYE
struct user *processExit(int argc, char *argv[], struct user *connList, int fd) {
  struct user *conn = get_user(connList, fd, NULL);
  
  if (conn == NULL) {
    char *ret = "GOODBYE \"stranger\" (No session created)\n";
    writeToFd(fd, ret);
    return connList;
  }
  
  conn->outbuffer = mergeStrings(6, SYS_MSG, "Goodbye ", conn->uname, "\n", time_now(), "\n");
  conn->out_len++;

  // connList = conn_remove(&connList, conn); // don't remove from list until we've written to the client

  return connList;
}


struct user *processMessage(char *message, struct user *connList, int fd){
  char *tokens[MAX_N_TOKENS] = {CMD_NOOP};
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

  // connList = processEscape(tokenIndex, tokens, connList, fd);

  char *command = tokens[0];

  if(strcmp(command, CMD_HELLO) == 0 )
    return processHello(tokenIndex, tokens, connList, fd);

  struct user *conn = get_user(connList, fd, NULL);
  if( conn == NULL ) {
    char *err = "error: You must say HELLO before sending messages\n";
    writeToFd(fd, err);
    return connList;
  }

  if( strcmp(command, CMD_EXIT) == 0 ) 
    connList = processExit(tokenIndex, tokens, connList, fd);
  // else if(strcmp(command, CMD_CHANNEL) == 0)
  //   connList = processChannel(tokenIndex, tokens, connList, fd);
  // else if(strcmp(command, CMD_DM) == 0)
  //   connList = processDM(tokenIndex, tokens, connList, fd);
  // else if(strcmp(command, CMD_HELP) == 0)
  //   connList = processHelp(tokenIndex, tokens, connList, fd);
  // else if(strcmp(command, CMD_REQS) == 0)
  //   connList = processRequests(tokenIndex, tokens, connList, fd);
  else if( strcmp(command, CMD_NOOP) == 0 ){
    // connList = processRefresh(tokenIndex, tokens, connList, fd);
    conn->outbuffer = conn->outbuffer == NULL ? conn->outbuffer = mergeStrings(2, "", "") : conn->outbuffer;
    // CMD_NOOP;

  }
  else if(strcmp(conn->channel, CHANL_GEN) == 0 || strcmp(conn->channel, CHANL_NICHE) == 0){
    update_buffers(connList, conn, tokens, tokenIndex);
  }
  // printf("channel in: %s\n== gen: %d\n==niche: %d\n", conn->channel, strcmp(conn->channel, CHANL_GEN), strcmp(conn->channel, CHANL_NICHE));
  // fflush(stdout);

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
  printf("\t%s uniqueKey\n", CMD_HELLO);
  printf("\t\ton success: sends '%s uniqueKey\\n' through socket\n", CMD_HELLO);
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texample:\n");
  printf("\t\t  %s bob\n\n", CMD_HELLO);
  printf("\t%s <channel-name>\n", CMD_CHANNEL);
  printf("\t\ton success: sends 'Now in channel: <channel-name>' from the system through socket\n");
  printf("\t\ton failure: sends error: message 'Channel <channel-name> does not exist' through socket\n");
  printf("\t\texamples:\n");
  printf("\t\t  %s general\n", CMD_CHANNEL);
  printf("\t\t  %s niche\n" CMD_CHANNEL);
  printf("\t%s <username>\n", CMD_DM);
  printf("\t\ton success: sens a message request to <username>\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texamples:\n");
  printf("\t\t  %s bob\n", CMD_DM);
  printf("\t\t  %s ana\n", CMD_DM);
  printf("\t%s\n", CMD_REQS);
  printf("\t\ton success: list the user's inbound messsage requests\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t%s<command>\n", CMD_ESC);
  printf("\t\ton success: allows one to send a command (except for empty string) as a standard message.\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t\texamples:\n");
  printf("\t\t  %s%s bob is how you'd DM bob\n", CMD_ESC, CMD_DM);
  printf("\t\t  %s%s is the requests command\n", CMD_ESC, CMD_REQS);
  printf("\t%s\n", CMD_HELP);
  printf("\t\ton success: sends this message\n");
  printf("\t\ton failure: also sends this message with error message\n");
  printf("\t''\n");
  printf("\t\tcommand is empty string, and refreshes the client to recieve messages.\n");
  printf("\t\ton success: sends pending messages to the client\n");
  printf("\t\ton failure: sends error: message through socket\n");
  printf("\t%s\n", CMD_EXIT);
  printf("\t\tnote: this will close an open connection opened with a %s command\n", CMD_HELLO);
  printf("\t\ton success: sends 'GOODBYE [username]\\n' through socket\n");
  printf("\t\ton failure: sends error: message through socket\n");
}

/// very good setup. It looks like everything works!  It looks like you included
///   dynamic-buffer.[ch] as requirements, but they were not required from the build.
///   Be sure to remove these from the Makefile and the header list of sum-server.c
///   -aaron
int main(int argc, char *argv[]){
  // if (argc != 2) {
  //   usage(argc, argv);
  //   exit(1);
  // }
  // setup socket and signal handlers
  // bind socket to port
  struct sockaddr_in address;
  struct in_addr ipAddress;

  address.sin_family = AF_INET;
  // address.sin_port = htons(atoi(argv[1]));
  address.sin_port = htons(TEMP_PORT);
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
            struct user *curr_user = get_user(clients, clientFds[currFd].fd, NULL);
            if (curr_user != NULL) {
              clients = conn_remove(&clients, curr_user);
            }
            // close(clientFds[currFd].fd);
            clientFds[currFd] = clientFds[numberClients - 1];
            numberClients -= 1;
            continue;
          }

          // Handle socket data when ready
          if(clientFds[currFd].revents & POLLIN ) {
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
              struct user *conn = get_user(clients, clientFds[currFd].fd, NULL);
              if (conn != NULL) clients = conn_remove(&clients, conn);
              
              // close(clientFds[currFd].fd);
              clientFds[currFd] = clientFds[numberClients - 1];
              numberClients -= 1;
            }
          }
          if(clientFds[currFd].revents & POLLOUT ) { // write to socket if nonblocking
            for(int i = 0; i < numberClients; i++){
              struct user *curr_user;
              if(clientFds[i].revents & POLLOUT && (curr_user = get_user(clients, clientFds[i].fd, NULL)) && curr_user->outbuffer != NULL){
                writeToFd(clientFds[i].fd, curr_user->outbuffer);
                curr_user->outbuffer = NULL;
                curr_user->out_len = 0;
                // clientFds[i].events &= ~POLLOUT;
                clientFds[i].events = POLLIN;
                poll(clientFds, numberClients, 0);
              }
            }
            // struct user *currClient = get_user(clients, clientFds[currFd].fd);
            // if(currClient == NULL){
            //   clientFds[currFd].events &= ~POLLOUT;
            //   continue;
            // }
            // int bytesWritten = writeToFd(currClient->fd, currClient->outbuffer);
            // if (bytesWritten == -1) {
            //   perror("write");
            // }
            // while (bytesWritten > 0) {
            //   bytesWritten = writeToFd(currClient->fd, currClient->outbuffer);
            //   currClient->outbuffer = &currClient->outbuffer[bytesWritten];
            // }
            // // clientFds[currFd].events = POLLIN;
            // clientFds[currFd].events |= POLLIN;
            // currClient->outbuffer = NULL;
          }
        }
        currFd += 1;
      }
    }

    close(clientFds[0].fd);
    free(clientFds);

    return 0;
}
