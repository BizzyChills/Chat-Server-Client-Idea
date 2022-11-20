
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "user.h"


void release(struct user *connection) {
  free(connection->uname);

  if (connection->outbuffer)
    free(connection->outbuffer);

  for(int i = 0; i < connection->reqs_len; i++) {
    if (connection->req_from[i])
      free(connection->req_from[i]);
    if (connection->reqs[i])
      free(connection->reqs[i]);
  }

  close(connection->fd);
  free(connection);
}

struct user *conn_create(const char *uname, int fd) {
  struct user *newConn = (struct user *) malloc(sizeof(struct user));

  newConn->uname = (char *) malloc(strlen(uname) * sizeof(char) + 1);
  newConn->uname = strcpy(newConn->uname, uname);
  newConn->fd = fd;
  newConn->channel = CHANL_GEN; // current channel. default is general
  newConn->reqs_len = 0; // length of the request list
  newConn->out_len = 0; // number of messages stored in outbuffer, not the size of the buffer
  asprintf(&newConn->outbuffer, "System:\n\tHello %s! Welcome to the server!\n%s\n", uname, system("date +%T"));
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

  while( it != NULL && !uname) {
    if(it->fd == fd ){
      return it;
    }

    it = it->next;
  }

  while( it != NULL && uname) {
    if( strcmp(it->uname, uname) == 0 ) {
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

void update_buffers(struct user *conn, const char *message, const char *channel) {
  while(conn){
    if(conn->out_len == MAX_OUT) conn = conn->next;
    if(conn->channel != channel) conn = conn->next;
    conn->outbuffer = mergeStrings(conn->outbuffer, message);
    conn->out_len++;
    conn = conn->next;
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