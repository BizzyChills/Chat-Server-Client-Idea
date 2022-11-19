
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "connection.h"

void release(struct connection *connection) {
  free(connection->key);
  if (connection->outBuffer)
    free(connection->outBuffer);
    
  for(int i = 0; i < connection->n_stored; i++) {
    if (connection->vars[i])
      free(connection->vars[i]);
  }
  close(connection->fd);
  free(connection);
}

struct connection *conn_create(const char *key, int fd) {
  struct connection *newConn = (struct connection *) malloc(sizeof(struct connection));

  newConn->key = (char *) malloc(strlen(key) * sizeof(char) + 1);
  newConn->key = strcpy(newConn->key, key);
  newConn->fd = fd;
  newConn->next = NULL;
  newConn->prev = NULL;
  newConn->n_stored = 0;

  return newConn;
}

struct connection *conn_insert(struct connection **list, struct connection *newConnection) {
  if( *list != NULL ) {
    newConnection->next = *list;
    (*list)->prev = newConnection;
  }

  newConnection->prev = NULL;
  return newConnection;
}

struct connection *conn_remove(struct connection **list, struct connection *toRemove) {
  if( toRemove->prev == NULL && toRemove->next == NULL ) {
    if( *list == toRemove) {
      *list = NULL;
    }

    release(toRemove);

    return *list;
  }

  if( toRemove->prev == NULL ) {
    struct connection *newHead = toRemove->next;
    newHead->prev = NULL;
    release(toRemove);
    return newHead;
  }

  if( toRemove->next == NULL ) {
    struct connection *newTail = toRemove->prev;
    newTail->next = NULL;
    release(toRemove);
    return *list;
  }

  toRemove->prev->next = toRemove->next;
  toRemove->next->prev = toRemove->prev;
  release(toRemove);
  return *list;
}

struct connection *conn_get(struct connection *list, const int fd) {
  struct connection *it = list;

  while( it != NULL ) {
    if(it->fd == fd){
      return it;
    }

    it = it->next;
  }

  return NULL;
}

void conn_fprint(FILE* file, struct connection *list) {
  if( file == NULL ) {
    fprintf(stderr, "error: conn_fprint: null file\n");
    return;
  }

  fprintf(file, "[\n");

  while( list != NULL ) {
    fprintf(file, "  {\n");
    fprintf(file, "    key: \"%s\"\n", list->key);
    fprintf(file, "    fd: %d\n", list->fd);
    fprintf(file, "    prev: %p\n", list->prev);
    fprintf(file, "    next: %p\n", list->next);
    fprintf(file, "  },\n");
    list = list->next;
  }

  fprintf(file, "]\n");
}

int conn_var_index(struct connection *conn, const int fd, char *var){

  int index = 0;
  struct connection *it = conn;

  while(index < it->n_stored && strcmp(it->vars[index], var) != 0){
    index++;
  }

  if( index == conn->n_stored && conn->n_stored == MAX_VARS) return -1; // cannot make more vars
  
  return index;
}