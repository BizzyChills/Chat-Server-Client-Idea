
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

  for(int i = 0; i < connection->out_len; i++) {
    if (connection->outbuffer[i])
      free(connection->outbuffer[i]);
  }

  close(connection->fd);
  free(connection);
}

struct user *conn_create(const char *uname, int fd) {
  struct user *newConn = (struct user *) malloc(sizeof(struct user));

  newConn->uname = (char *) malloc(strlen(uname) * sizeof(char) + 1);
  newConn->uname = strcpy(newConn->uname, uname);
  newConn->fd = fd;

  asprintf(&newConn->outbuffer[0], "%sHello %s! Welcome to the server!\n%s", SYS_MSG, uname, time_now());
  newConn->out_rank[0] = 0;

  newConn->channel = mergeStrings(2, CHANL_GEN, "\0"); // current channel. default is general
  // char *tmp;
  // asprintf(&tmp, "%sNow in channel: %s\n%s", SYS_MSG, CHANL_GEN, time_now());
  asprintf(&newConn->outbuffer[1], "%sNow in channel: %s\n%s", SYS_MSG, CHANL_GEN, time_now());

  // newConn->outbuffer[0] = mergeStrings(2, newConn->outbuffer[0], tmp);
  newConn->out_len = 2; // number of messages stored in outbuffer, not the size of the buffer

  newConn->reqs_len = 0; // length of the request list
  newConn->next = NULL;
  newConn->prev = NULL;

  // free(tmp);

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

struct user *remove_buffer(struct user *conn, int index){
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

struct user *pack_buffers(struct user *connList){
  // for each user in the list
  // merge all the messages of their buffer into one string

  struct user *it = connList;
  while(it != NULL){
    if(it->out_len > 1){
      char *tmp = it->outbuffer[0];
      for(int i = 1; i < it->out_len; i++){
        tmp = mergeStrings(2, tmp, it->outbuffer[i]);
        free(it->outbuffer[i]);
        it->outbuffer[i] = NULL;
      }
      free(it->outbuffer[0]);
      it->outbuffer[0] = tmp;
      it->out_len = 1;
    }
    it = it->next;
  }
  return connList;
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

    if(len == MAX_OUT){
      for(int i = 0; i < MAX_OUT - 1; i++){
        if(connList->out_rank[i] == 3){
          remove_buffer(connList, i);
          connList->outbuffer[i] = *message;
          connList->out_rank[i] = 3;
          break;
        }
      }
      connList = connList->next;
    }
    if(strcmp(connList->channel, channel) != 0){
      connList = connList->next;
      continue;
    }
    else{
      connList->outbuffer[len] = mergeStrings(4, from->uname, ":\n    ", *message, time_now());
      connList->out_rank[len] = 3;
      connList->out_len++;
      connList = connList->next;
    }
  }
}

void *insert_buffer(struct user *conn, char *message, const int rank){
  int len = conn->out_len;
  if(len == MAX_OUT){ // if the buffer is full
    int overw = get_rank_i(conn);
    if(conn->out_rank[overw] == RANK_SYS && rank != RANK_SYS || (conn->out_rank[overw] == RANK_PRIV && rank == RANK_PUB)) return conn; // overwrite priority fail
    
    conn = remove_buffer(conn, overw);
    len = len-1;
  }
  
  conn->outbuffer[len] = message;
  conn->out_rank[len] = rank;
  conn->out_len = len == MAX_OUT ? MAX_OUT : len + 1;
  return conn;
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