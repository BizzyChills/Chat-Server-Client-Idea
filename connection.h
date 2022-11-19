
#ifndef CONNECTION_H
#define CONNECTION_H
#define MAX_VARS 64

#include <stdio.h>

struct connection {
  char *key;
  int fd;
  struct connection *next;
  struct connection *prev;
  char *vars[MAX_VARS];
  int vals[MAX_VARS];
  int n_stored;
  char *outBuffer;
};

struct connection *conn_create(const char *key, int fd);
struct connection *conn_get(struct connection *list, const int fd);
struct connection *conn_insert(struct connection **list, struct connection *newConnection);
struct connection *conn_remove(struct connection **list, struct connection *toRemove);
void conn_fprint(FILE* file, struct connection *list);
int conn_var_index(struct connection *conn, const int fd, char *var);

#endif
