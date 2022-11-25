
#include "user.h"


void release(struct user *connList, struct user *connection) {
  struct user *conn;
  if(connection->channel){ // if the user is in a DM, remove their request to the other user
    if(strcmp(connection->channel, CHANL_GEN) != 0 && strcmp(connection->channel, CHANL_NICHE) != 0 && req_remove(connection, (conn = get_user(connList, -1, connection->channel))) == 0){
      insert_buffer(conn, mergeStrings(3, "error: ", conn->channel, " has disconnected from the server\n\n"), RANK_SYS);
      free(conn->channel);
      conn->channel = malloc(strlen(CHANL_GEN) + 1);
      strcpy(conn->channel, CHANL_GEN);
      insert_buffer(conn, mergeStrings(5, SYS_MSG, "Now in channel: ", conn->channel, "\n", time_now()), RANK_SYS);
    }
    free(connection->channel);
  }

  for(int i = 0; i < connection->reqs_len; i++) {
    if (connection->reqs_from[i]){
      conn = get_user(connList, -1, connection->reqs_from[i]);
      if(conn) { // if there is a user in DM with this user, notify them that the user has left and change their channel to general
        insert_buffer(conn, mergeStrings(3, "error: ", conn->channel, " has disconnected from the server\n\n"), RANK_SYS);
        free(conn->channel);
        conn->channel = malloc(strlen(CHANL_GEN) + 1);
        strcpy(conn->channel, CHANL_GEN);
        insert_buffer(conn, mergeStrings(5, SYS_MSG, "Now in channel: ", conn->channel, "\n", time_now()), RANK_SYS);
      }
      free(connection->reqs_from[i]);
    }
  }

  for(int i = 0; i < connection->out_len; i++) {
    if (connection->outbuffer[i])
      free(connection->outbuffer[i]);
  }

  if(connection->uname) free(connection->uname);
  if(connection->help_buffer) free(connection->help_buffer);
  close(connection->fd);
  free(connection);
}

struct user *conn_create(const char *uname, int fd) {
  struct user *newConn = (struct user *) malloc(sizeof(struct user));

  newConn->uname = (char *) malloc(strlen(uname) * sizeof(char) + 1);
  newConn->uname = strcpy(newConn->uname, uname);
  newConn->fd = fd;
  newConn->help_buffer = mergeStrings(11, 
    "Commands:\n\t", 
    CMD_HELLO, " <username> - create a new session\n\t", 
    CMD_EXIT, " - end your session\n\t", 
    CMD_CHANL, " <channel> - switch to a different channel ('general' or 'niche')\n\t", 
    CMD_DM, " [username] - send a private message to the specified user.\n\t\t\tIf no username specified, list all users in the current channel\n\t\t\tIf username is own username, lists all private message requests\n\t", 
    CMD_HELP, " - display this help message\n\n");

  newConn->need_help = 0; // help message print or not

  newConn->reqs_notify = 1;

  asprintf(&newConn->outbuffer[0], "%sHello %s! Welcome to the server!\n%s", SYS_MSG, uname, time_now());
  newConn->out_rank[0] = 0;

  newConn->channel = malloc(strlen(CHANL_GEN) + 1);
  strcpy(newConn->channel, CHANL_GEN); // current channel. default is general

  asprintf(&newConn->outbuffer[1], "%sNow in channel: %s\n%s", SYS_MSG, CHANL_GEN, time_now());

  newConn->out_len = 2; // number of messages stored in outbuffer, not the size of the buffer

  newConn->reqs_len = 0; // length of the request list
  newConn->next = NULL;
  newConn->prev = NULL;


  return newConn;
}

struct user *conn_insert(struct user **list, struct user *newConnection) {
  if( *list != NULL ) {
    newConnection->next = *list;
    (*list)->prev = newConnection;
  }

  newConnection->prev = NULL;
  return newConnection;
}

struct user *conn_remove(struct user **list, struct user *toRemove) {
  if( toRemove->prev == NULL && toRemove->next == NULL ) {
    if( *list == toRemove) {
      *list = NULL;
    }

    release(*list, toRemove);

    return *list;
  }

  if( toRemove->prev == NULL ) {
    struct user *newHead = toRemove->next;
    newHead->prev = NULL;
    release(*list, toRemove);
    return newHead;
  }

  if( toRemove->next == NULL ) {
    struct user *newTail = toRemove->prev;
    newTail->next = NULL;
    release(*list, toRemove);
    return *list;
  }

  toRemove->prev->next = toRemove->next;
  toRemove->next->prev = toRemove->prev;
  release(*list, toRemove);
  return *list;
}

struct user *get_user(struct user *list, const int fd, const char *uname) {
  struct user *it = list;
  while( it != NULL && uname != NULL) { // search by uname
    if( strcmp(it->uname, uname) == 0 ) {
      return it;
    }
    it = it->next;
  }

  it = list;
  while( it != NULL && uname == NULL ) { // search by fd
    if(it->fd == fd ){
      return it;
    }

    it = it->next;
  }

  return NULL;
}

struct user *empty_buffer(struct user *conn) {
  for(int i = 0; i < conn->out_len; i++) {
    if (conn->outbuffer[i]){
      free(conn->outbuffer[i]);
      conn->out_rank[i] = -1;
    }
  }
  conn->out_len = 0;
  return conn;
}

struct user *remove_buffer_i(struct user *conn, int index){

  if(index < 0 || index >= conn->out_len) return conn;

  if(conn->out_len == 1){
    if(conn->outbuffer[index])
      free(conn->outbuffer[index]);
    
    conn->outbuffer[index] = NULL;
    conn->out_rank[index] = -1;
    conn->out_len = 0;
    return conn;
  }

  while(index < conn->out_len - 1){
    if(conn->outbuffer[index])
      free(conn->outbuffer[index]); // free the original string
    conn->outbuffer[index] = conn->outbuffer[index + 1]; // reassign the pointer to the next string

    conn->out_rank[index] = conn->out_rank[index + 1]; // reassign the rank

    index++;
    
    conn->outbuffer[index] = NULL; // and nullify
    conn->out_rank[index] = -1; // set the next rank to null since it moved back one
  }
  conn->out_len--;
  return conn;
}

struct user *remove_buffer_rank(struct user *conn, int rank){
  for(int i = 0; i < conn->out_len; i++){
    if(conn->out_rank[i] == rank){
      conn = remove_buffer_i(conn, i);
    }
  }
  return conn;
}

struct user *pack_buffers(struct user *connList){
  // for each user in the list
  // merge all the messages of their buffer into one string

  struct user *it = connList;
  while(it != NULL){
    if(it->out_len > 1){
      char *tmp = it->outbuffer[0]; // malloced string
      
      for(int i = 1; i < it->out_len; i++){
        tmp = mergeStrings(2, tmp, it->outbuffer[i]);
        
        if(it->outbuffer[i])
          free(it->outbuffer[i]);
        it->outbuffer[i] = NULL;
      }

      if(it->outbuffer[0])
        free(it->outbuffer[0]);

      it->outbuffer[0] = tmp;
      it->out_len = 1;
    }
    it = it->next;
  }
  return connList;
}

struct user *insert_buffer(struct user *conn, char *message, const int rank){
  int len = conn->out_len;
  if(len == MAX_OUT){ // if the buffer is full
    int overw = get_rank_i(conn);
    if(conn->out_rank[overw] == RANK_SYS && rank != RANK_SYS || (conn->out_rank[overw] == RANK_PRIV && rank == RANK_PUB)) return conn; // overwrite priority fail
    
    conn = remove_buffer_i(conn, overw);
    len = len-1;
  }
  
  conn->outbuffer[len] = message;
  conn->out_rank[len] = rank;
  conn->out_len = len == MAX_OUT ? MAX_OUT : len + 1;
  return conn;
}

// void conn_fprint(FILE* file, struct user *list) {
//   if( file == NULL ) {
//     fprintf(stderr, "error: conn_fprint: null file\n");
//     return;
//   }
// 
//   fprintf(file, "[\n");
// 
//   while( list != NULL ) {
//     fprintf(file, "  {\n");
//     fprintf(file, "    uname: \"%s\"\n", list->uname);
//     fprintf(file, "    fd: %d\n", list->fd);
//     fprintf(file, "    prev: %p\n", list->prev);
//     fprintf(file, "    next: %p\n", list->next);
//     fprintf(file, "  },\n");
//     list = list->next;
//   }
// 
//   fprintf(file, "]\n");
// }

int req_remove(struct user *from, struct user *to){ // remove from from to's requests
  for(int i = 0; i < to->reqs_len; i++){ // and shift the rest of the requests back
    if(strcmp(to->reqs_from[i], from->uname) == 0){ // found target, remove
      to->reqs_from[i] = NULL;
      while(i < to->reqs_len - 1){
        to->reqs_from[i] = to->reqs_from[i + 1];
        i++;
      }
      to->reqs_len--;
      return 1;
    }
  }
  return 0;
}

int DM_notify(struct user *connList, struct user *from, struct user *to){

  if(to->reqs_len == MAX_PRIV){
    insert_buffer(from, mergeStrings(2, to->uname, "'s inbox is full\n"), RANK_PRIV); // inbox full
    return 0;
  }

  if(strcmp(to->channel, from->uname) == 0){
    return req_remove(to, from); //remove to, from from's reqs because to is already in from's channel
  } // user is already in DM

  if(to->reqs_notify)
    insert_buffer(to, mergeStrings(8, SYS_MSG, from->uname, " has sent you a private message. Use '", CMD_DM, " ", from->uname, "' to open up a private channel with them\n", time_now()), RANK_PRIV);
  
  to->reqs_from[to->reqs_len] = malloc(strlen(from->uname) + 1);
  strcpy(to->reqs_from[to->reqs_len], from->uname);
  to->reqs_len++;

  return 1;
}

int get_rank_i(struct user*conn){
  int rank_i = -1;
  for(int i = 0; i < conn->out_len; i++){
    if(conn->out_rank[i] > rank_i) rank_i = i;
  }
  return rank_i;
}

void update_buffers(struct user *connList, struct user *from, char *message[], const int argc) {
  char *channel = from->channel;
  int message_i = 1;

  while(message_i < argc)
    *message = mergeStrings(3, *message, ARGS_DELIM_STR, message[message_i++]);

  *message = mergeStrings(2, *message, "\n");

  while(connList){
    int len = connList->out_len;

    if(len == MAX_OUT){ // check for overwite priority
      for(int i = 0; i < MAX_OUT - 1; i++){
        if(connList->out_rank[i] == RANK_PUB){
          remove_buffer_i(connList, i);
          connList->outbuffer[i] = *message;
          connList->out_rank[i] = RANK_PUB;
          break;
        }
      }
      connList = connList->next;
    }
    if(strcmp(connList->channel, channel) != 0 && strcmp(connList->uname, from->channel) != 0){ // not in the same channel, and not being delivered a DM
      connList = connList->next;
      continue;
    }
    else{
      connList->outbuffer[len] = mergeStrings(4, from->uname, ":\n    ", *message, time_now());
      connList->out_rank[len] = strcmp(connList->uname, from->channel) != 0 ? RANK_PUB : RANK_PRIV;
      connList->out_len++;
      connList = connList->next;
    }
  }
}



// int write_clients(struct user *conn) {
//   int err = 0;
// 
//   while(conn){ 
//     int len = strlen(conn->outbuffer);
//     int totalBytesWritten = 0;
//     do {
//       int bytesWritten = write(conn->fd, conn->outbuffer + totalBytesWritten, len - totalBytesWritten);
//       if( bytesWritten == -1 ) {
//         perror("writeToFd|write");
//         err = -1;
//       }
// 
//       totalBytesWritten += bytesWritten;
//     } while(totalBytesWritten < len);
//     conn = conn->next;
//   }
//   return err;
// }