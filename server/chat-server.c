#define _XOPEN_SOURCE 700 // allows use of sigaction
#define _GNU_SOURCE
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
#include <math.h>
#include <stdio.h>

#include "commands.h"
#include "user.h"
#include "utils.h"

#define BUFFER_SIZE 4096
#define MAX_N_TOKENS 10
#define MAX_CONNECTIONS 1024
#define RANK_SYS 0
#define RANK_PRIV 1 // priorities for output messages
#define RANK_PUB 2
// #define TEMP_PORT 57300
#define RESERVED_LEN 11 // number of reserved unames

const char *reserved_unames[RESERVED_LEN] = {"system", "server", "admin", "admins", "root", "administrator", "administrators", "moderator", "moderators", "mod", "mods"};

int writeToFd(int fd, const char *message) {
  if(strcmp(message, "") == 0) message = "\r";

  int len = strlen(message);
  char *len_str = itoa(len);
  
  int totalBytesWritten = 0;
 // write length of message to fd
  do {
    int bytesWritten = write(fd, len_str + totalBytesWritten, strlen(len_str) - totalBytesWritten);
    if( bytesWritten == -1 ) {
      perror("writeToFd|write");
      return -1;
    }

    totalBytesWritten += bytesWritten;
    // printf("bytes written: %d\n", totalBytesWritten);
    // fflush(stdout);
  } while(totalBytesWritten < strlen(len_str));

  useconds_t s = 2000; // give the client time to read the length
  usleep(s);
  if(len_str)
    free(len_str);

  totalBytesWritten = 0;

  do {
    int bytesWritten = write(fd, message + totalBytesWritten, len - totalBytesWritten);
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

  if( (conn = get_user(connList, fd, NULL)) != NULL ) { // if you have said hello before
    update_buffers(connList, conn, argv, argc);
    return connList;
  }

  if( argc != 2) {
    char *err = "error: expected 'HELLO [uname]'\n";
    writeToFd(fd, err); // write error to client, can't add to outbuffer because we don't have a connection yet
    return connList;
  }

  if( strlen(username) > MAX_UNAME || strlen(username) < MIN_UNAME ) {
    char *err;
    asprintf(&err, "error: username must be between %d and %d characters\n", MIN_UNAME, MAX_UNAME);
    writeToFd(fd, err); // write error to client, can't add to outbuffer because we don't have a connection yet
    free(err);
    return connList;
  }

  if( get_user(connList, -1, username) != NULL ) {
    char *err;
    asprintf(&err, "error: username '%s' is already taken\n", username);
    writeToFd(fd, err); // write error to client, can't add to outbuffer because we don't have a connection yet
    free(err);
    return connList;
  }
  
  char *tmp_uname = malloc(strlen(username) + 1);
  strcpy(tmp_uname, username);
  
  // convert tmp_uname to lowercase for reserved uname check
  for(int i = 0; i < strlen(tmp_uname); i++) tmp_uname[i] = tolower(tmp_uname[i]);

  for(int i = 0; i < RESERVED_LEN; i++) {
    if( strcmp(tmp_uname, reserved_unames[i]) == 0 ) {
      char *err;
      asprintf(&err, "error: username '%s' is reserved\n", username);
      writeToFd(fd, err); // write error to client, can't add to outbuffer because we don't have a connection yet
      free(err);
      free(tmp_uname);
      return connList;
    }
    for(int x = 0; tmp_uname[x]; x++){
      char n = tmp_uname[x];
      if(('A' < n || n > 'Z') && ('a' > n || n > 'z') && ('0' > n || n > '9')){
        char *err;
        err = mergeStrings(2, "error: ", "username must be alphanumeric (American English) and cannot contain any whitespace\n");
        writeToFd(fd, err); // write error to client, can't add to outbuffer because we don't have a connection yet
        free(err);
        free(tmp_uname);
        return connList;
      }
    }
  }

  if(tmp_uname)
    free(tmp_uname);

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
  empty_buffer(conn);
  conn->outbuffer[0] = mergeStrings(6, SYS_MSG, "Goodbye ", conn->uname, "\n", time_now(), "\n");
  conn->out_rank[0] = RANK_SYS;
  conn->out_len = 1;

  // connList = conn_remove(&connList, conn); // don't remove from list until we've written to the client

  return connList;
}

void processChannel(int argc, char *argv[], struct user *connList, int fd){
  struct user *conn = get_user(connList, fd, NULL);

  if(conn == NULL){
    char *err;
    asprintf(&err, "error: you must say %s before you can join a channel\n", CMD_HELLO);
    writeToFd(fd, err);
    free(err);
    return;
  }

  if (argc < 2) {
    insert_buffer(conn, mergeStrings(6, SYS_MSG, "Currently in channel ", CHANL_GEN, "\n", time_now(), "\n"), RANK_SYS);
    return;
  }
  char *channel = argv[1];

  if (strcmp(channel, CHANL_GEN) == 0){ // switching to general
    if (strcmp(conn->channel, CHANL_GEN) == 0){ // already in general
      char *err;
      asprintf(&err, "error: already in channel '%s'\n", CHANL_GEN); // DONT FREE, freed after write
      insert_buffer(conn, err, RANK_SYS);
      return;
    }
    else{
      if(strcmp(conn->channel, CHANL_NICHE) != 0){ // are currently in a DM
        req_remove(conn, get_user(connList, -1, conn->channel));
      }
      free(conn->channel);
      conn->channel = malloc(strlen(CHANL_GEN) + 1);
      strcpy(conn->channel, CHANL_GEN);
      remove_buffer_rank(conn, RANK_PUB); // remove previous channel's messages from buffer 
      insert_buffer(conn, mergeStrings(5, SYS_MSG, "Now in channel: ", CHANL_GEN, "\n", time_now()), RANK_SYS);
    }
  }

  else if (strcmp(channel, CHANL_NICHE) == 0){ // switching to niche
    if (strcmp(conn->channel, CHANL_NICHE) == 0){ // already in niche
      char *err;
      asprintf(&err, "error: already in channel '%s'\n", CHANL_NICHE);
      insert_buffer(conn, err, RANK_SYS);
      return;
    }
    else{
      if(strcmp(conn->channel, CHANL_GEN) !=0){ // are currently in a DM
        req_remove(conn, get_user(connList, -1, conn->channel));
      }
      free(conn->channel);
      conn->channel = malloc(strlen(CHANL_NICHE) + 1);
      strcpy(conn->channel, CHANL_NICHE);
      remove_buffer_rank(conn, RANK_PUB); // remove previous channel's messages from buffer 
      insert_buffer(conn, mergeStrings(5, SYS_MSG, "Now in channel: ", CHANL_NICHE, "\n", time_now()), RANK_SYS);
    }
  }
  else{
    insert_buffer(conn, mergeStrings(3, "error: Unknown channel '", channel, "'\n\n"), RANK_SYS);
  }
}

void channel_list(struct user *connList, struct user *conn, char *user_list){
  user_list = mergeStrings(4, SYS_MSG, "Users currently in channel '", conn->channel, "':\n");
  
  char *tmp_str;
  struct user *tmp_usr = connList;
  
  while(tmp_usr){
    if(strcmp(tmp_usr->channel, conn->channel) == 0){ // grab all users in current channel
      tmp_str = mergeStrings(4, user_list, "\t", tmp_usr->uname, "\n");
      free(user_list);
      user_list = tmp_str; 
    }

    tmp_usr = tmp_usr->next;
  }

  tmp_str = mergeStrings(2, user_list, time_now()); // add timestamp to end of user list
  free(user_list);
  user_list = tmp_str;
  
  insert_buffer(conn, user_list, RANK_SYS); // add user list to buffer
  return;
}

void processDM(int argc, char *argv[], struct user *connList, int fd){
  struct user *conn = get_user(connList, fd, NULL);
  struct user *tmp_usr;
  
  if (argc == 1 || strcmp(argv[1], conn->uname) == 0) { // no arg to list users in channel. own uname to list users in private chat with you
    char *user_list;

    if (argc == 1){
      if(strcmp(conn->channel, CHANL_GEN) == 0 || strcmp(conn->channel, CHANL_NICHE) == 0) channel_list(connList, conn, user_list); // list users in channel

      else insert_buffer(conn, mergeStrings(5, SYS_MSG, "You are currently in a private chat with ", conn->channel, "\n", time_now()), RANK_PRIV);// list whose private you're in 
      
      return;
    }
    
    user_list = mergeStrings(2, SYS_MSG, "Users in your DMs:\n");

    char *tmp_str;

    for(int i = 0; i < conn->reqs_len; i++){ // grab all users in private chat waiting for you
      tmp_str = mergeStrings(4, user_list, "\t", conn->reqs_from[i], "\n");
      free(user_list);
      user_list = tmp_str; 
    }

    tmp_str = mergeStrings(2, user_list, time_now()); // add timestamp to end of user list
    free(user_list);
    user_list = tmp_str;
    
    insert_buffer(conn, user_list, RANK_PRIV); // add user list to buffer
    return;
  }
  
  char *uname = argv[1];

  if(strcmp(uname, "!") == 0){
    conn->reqs_notify = !conn->reqs_notify;
    insert_buffer(conn, mergeStrings(5, SYS_MSG, "You will ", conn->reqs_notify ? "now" : "no longer", " be notified of private chat requests\n", time_now()), RANK_SYS);
    return;
  }
  struct user *target = get_user(connList, -1, uname);

  if (argc > 2) {
    insert_buffer(conn, mergeStrings(4, "error: expected '", CMD_DM, uname, "'\n"), RANK_PRIV);
    return;
  }

  if (target == NULL){ // target doesn't exist
    insert_buffer(conn, mergeStrings(3, "error: user '", uname, "' does not exist\n"), RANK_PRIV);
    return;
  }

  if(strcmp(conn->channel, uname) == 0){ // already in DM with target
    insert_buffer(conn, mergeStrings(3, "error: already in ", uname, "'s DMs\n"), RANK_PRIV);
  }
  
  
  else if(DM_notify(connList, conn, target)){ // check if DM is possible
    remove_buffer_rank(conn, RANK_PUB); // remove previous channel's messages from buffer 
    remove_buffer_rank(conn, RANK_PRIV); // remove previous private channel's messages from buffer 
    free(conn->channel);
    conn->channel = malloc(strlen(uname) + 1);
    strcpy(conn->channel, uname);
    
    insert_buffer(conn, mergeStrings(5, SYS_MSG, "Now in DM with ", uname, "\n", time_now()), RANK_SYS);
  }
  return;

}

void usage(int argc, char *argv[]) {
  if(argc!=2) return printf("usage: %s <port-number>\n", argv[0]);
  
  printf("\tCtrl + C to quit\n\n");

  printf("Supports the following commands:\n");

  printf("\t%s <username>\n", CMD_HELLO);
  printf("\t    on success: sends '%s <username>' through socket\n", CMD_HELLO);
  printf("\t    on failure: sends error: message through socket\n");
  printf("\t    example:\n");
  printf("\t\t%s bob\n\n", CMD_HELLO);

  printf("\t%s [channel-name]\n", CMD_CHANL);
  printf("\t    if no channel name is given, prints the current channel. Otherwise, switches user to specified channel.\n");
  printf("\t    on success: sends 'Now in channel: <channel-name>' from the system through socket\n");
  printf("\t    on failure: sends error: message 'Channel <channel-name> does not exist' through socket\n");
  printf("\t    examples:\n");
  printf("\t\t%s %s\n", CMD_CHANL, CHANL_GEN);
  printf("\t\t%s %s\n\n", CMD_CHANL, CHANL_NICHE);

  printf("\t%s [username]\n", CMD_DM);
  printf("\t    if [username] is caller's username, lists people waiting in caller's DMs. Otherwise, enter [username]'s DMs\n");
  printf("\t    on success: sends a message that user is in DMs\n");
  printf("\t    on failure: sends error: message through socket\n\n");
  printf("\t    if no username is given:\n");
      printf("\t\tIf in a public channel, lists all users in that channel.\n");
      printf("\t\tOtherwise, lists the user you are messaging.\n\n");
  printf("\t    examples:\n");
    printf("\t\t%s bob\n", CMD_DM);
    printf("\t\t%s ana\n\n", CMD_DM);

  printf("\t%s<command>\n", CMD_ESC);
  printf("\t    on success: allows one to send a command as a standard message (or %s itself)\n", CMD_ESC);
  printf("\t    on failure: sends error: message through socket\n");
  printf("\t    examples:\n");
  printf("\t\t%s%s bob is how you'd DM bob\n", CMD_ESC, CMD_DM);
  printf("\t\t%s%s is how you escape the escape character\n\n", CMD_ESC, CMD_ESC);

  printf("\t%s\n", CMD_HELP);
  printf("\t    on success: sends a simplified version of this message\n");
  printf("\t    on failure: also sends said message with error message\n\n");

  printf("\t''\n");
  printf("\t    note: command is empty string, and refreshes the client to recieve messages\n");
  printf("\t    techincally, any message recieved from the client refreshes, but the empty string has no side-effects\n");
  printf("\t    on success: sends pending messages to the client\n");
  printf("\t    on failure: sends error: message through socket\n\n");

  printf("\t%s\n", CMD_EXIT);
  printf("\t    note: this will close an open connection opened with a %s command\n", CMD_HELLO);
  printf("\t    on success: sends 'GOODBYE [username]' through socket\n");
  printf("\t    on failure: sends error: message through socket\n");
  


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


  char *command = tokens[0];

  if(strcmp(command, CMD_HELLO) == 0 )
    return processHello(tokenIndex, tokens, connList, fd);

  struct user *conn = get_user(connList, fd, NULL);
  if(strcmp(command, CMD_HELP) == 0){
    if(conn == NULL){
      char *err = mergeStrings(11, 
      "Commands:\n", 
      CMD_HELLO, " <username> - create a new session\n", 
      CMD_EXIT, " - end your session\n", 
      CMD_CHANL, " <channel> - switch to a different channel ('general' or 'niche')\n", 
      CMD_DM, " [username] - send a private message to the specified user.\n\t\t If no username specified, list all users in the current channel\n", 
      CMD_HELP, " - display this help message\n\n");
      writeToFd(fd, err);
      free(err);
      return connList;
    }

    conn->need_help = 1;
    return connList;
  }

  if( conn == NULL ) {
    char *err = "error: You must say HELLO before accessing the server\n";
    writeToFd(fd, err);
    return connList;
  }
  
  if( strcmp(command, CMD_NOOP) == 0 || (tokenIndex == 1 && strcmp(command, "%") == 0)){ // noop or single escape char
    insert_buffer(conn, mergeStrings(2, "", ""), RANK_PUB);
  }
  
  else if( strcmp(command, CMD_EXIT) == 0 )
    return processExit(tokenIndex, tokens, connList, fd);

  else if(strcmp(command, CMD_CHANL) == 0){
    processChannel(tokenIndex, tokens, conn, fd);
  }

  else if(strcmp(command, CMD_DM) == 0)
    processDM(tokenIndex, tokens, connList, fd);

  // else if(strcmp(command, CMD_REQS_NOTIFY) == 0)
  //   conn->reqs_notify = !conn->reqs_notify;
    // processRequests(tokenIndex, tokens, connList, fd);

  else if(strcmp(command, CMD_HELP) == 0){
    insert_buffer(conn, conn->help_buffer, RANK_SYS);
  }

  else {
    if(command[0] == *CMD_ESC){
      tokens[0] = &command[1];
      if(!tokens[0]){
        // tokens[0] malloc(sizeof(char));
        strcpy(tokens[0], "");
      }
    } 
    
    // if(strcmp(conn->channel, CHANL_GEN) == 0 || strcmp(conn->channel, CHANL_NICHE) == 0){
    update_buffers(connList, conn, tokens, tokenIndex);
    // }

  }

  if(messageCopy)
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


int main(int argc, char *argv[]){
  if (argc != 2) {
    usage(argc, argv);
    exit(1);
  }
  // setup socket and signal handlers
  // bind socket to port
  
  usage(argc, argv);

  struct sockaddr_in address;
  struct in_addr ipAddress;

  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[1]));
  // address.sin_port = htons(TEMP_PORT);
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
            else if( bytesRead > 0) { // successfully read some data, process command
              char *temp_buf = malloc(BUFSIZ);

              poll(&clientFds[currFd], 1, 0);

              while(clientFds[currFd].revents & POLLIN && read(clientFds[currFd].fd, temp_buf, BUFSIZ) > 0) {
                temp_buf = mergeStrings(2, &buffer, temp_buf);

                poll(&clientFds[currFd], 1, 0);
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
            clients = pack_buffers(clients);
            for(int i = 0; i < numberClients; i++){
              struct user *curr_user;

              if(clientFds[i].revents & POLLOUT && (curr_user = get_user(clients, clientFds[i].fd, NULL))){
                if(curr_user->need_help){
                  curr_user->need_help = 0;
                  writeToFd(clientFds[i].fd, curr_user->help_buffer);
                } 
                // only write help message if requested, don't include nor delete messages. 
                else if(curr_user->outbuffer[0] != NULL){
                  writeToFd(clientFds[i].fd, curr_user->outbuffer[0]);
                  curr_user = remove_buffer_i(curr_user, 0);
                }

                clientFds[i].events = POLLIN;
              }
            }
          }
        }
        currFd += 1;
      }
    }

    close(clientFds[0].fd);
    if(clientFds)
      free(clientFds);

    return 0;
}
