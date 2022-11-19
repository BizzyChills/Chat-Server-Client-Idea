
#ifndef USER_H
#define USER_H
#define MAX_PRIV 5

#include <stdio.h>

struct user {
  char *uname;
  int fd;
  char *channel;
  char *req_from[MAX_PRIV];
  char *reqs[MAX_PRIV];
  int reqs_len;
  char *outBuffer;
  struct user *next;
  struct user *prev;
};

struct user *conn_create(const char *uname, int fd);
struct user *conn_get(struct user *list, const int fd);
struct user *conn_insert(struct user **list, struct user *newConnection);
struct user *conn_remove(struct user **list, struct user *toRemove);
// void conn_fprint(FILE* file, struct user *list);
int conn_var_index(struct user *conn, char *from);

#endif
