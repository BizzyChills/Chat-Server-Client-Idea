
#include "user.h"


void release(struct user *connection) {
  free(connection->uname);
  free(connection->channel);

  for(int i = 0; i < connection->reqs_len; i++) {
    if (connection->req_from[i])
      free(connection->req_from[i]);
    if (connection->reqs[i])
      free(connection->reqs[i]);
  }

  if (connection->outbuffer)
    free(connection->outbuffer);

  close(connection->fd);
  free(connection);
}

struct user *conn_create(const char *uname, int fd) {
  struct user *newConn = (struct user *) malloc(sizeof(struct user));

  newConn->uname = (char *) malloc(strlen(uname) * sizeof(char) + 1);
  newConn->uname = strcpy(newConn->uname, uname);
  newConn->fd = fd;
  newConn->channel = malloc(strlen(CHANL_GEN)); // current channel. default is general
  newConn->channel = strcpy(newConn->channel, CHANL_GEN);
  newConn->reqs_len = 0; // length of the request list
  asprintf(&newConn->outbuffer, "%sHello %s! Welcome to the server!\n%s", SYS_MSG, uname, time_now());
  char *sys_msg = mergeStrings(5, SYS_MSG, "Now in channel: ", CHANL_GEN, "\n", time_now());
  newConn->outbuffer = mergeStrings(2, newConn->outbuffer, sys_msg);
  newConn->out_len = 1; // number of messages stored in outbuffer, not the size of the buffer
  newConn->next = NULL;
  newConn->prev = NULL;

  free(sys_msg);
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

    release(toRemove);

    return *list;
  }

  if( toRemove->prev == NULL ) {
    struct user *newHead = toRemove->next;
    newHead->prev = NULL;
    release(toRemove);
    return newHead;
  }

  if( toRemove->next == NULL ) {
    struct user *newTail = toRemove->prev;
    newTail->next = NULL;
    release(toRemove);
    return *list;
  }

  toRemove->prev->next = toRemove->next;
  toRemove->next->prev = toRemove->prev;
  release(toRemove);
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

int get_reqt(struct user *conn, char *from){

  int index = 0;
  struct user *it = conn;

  while(index < it->reqs_len && strcmp(it->req_from[index], from) != 0){
    index++;
  }

  // index = index == conn->reqs_len ? -1 : index;
  
  return index == conn->reqs_len ? -1 : index;
}

void update_buffers(struct user *connList, struct user *from, char *message[], const int argc) {
  char *channel = from->channel;
  int message_i = 1;
  while(message_i < argc)
    *message = mergeStrings(3, *message, ARGS_DELIM_STR, message[message_i++]);

  *message = mergeStrings(2, *message, "\n");

  while(connList){
    if(connList->out_len == MAX_OUT) connList = connList->next;
    if(strcmp(connList->channel, channel) != 0) connList = connList->next;
    if(connList->outbuffer != NULL) connList->outbuffer = mergeStrings(3, connList->outbuffer, "\n", from->uname);
    else{
      connList->outbuffer = malloc(strlen(from->uname) * sizeof(char) + 1);
      connList->outbuffer = strcpy(connList->outbuffer, from->uname);
    }
    
    connList->outbuffer = mergeStrings(4, connList->outbuffer, ":\n    ", *message, time_now());
    
    connList->out_len++;
    connList = connList->next;
  }
  free(*message);
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