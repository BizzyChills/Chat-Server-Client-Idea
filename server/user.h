#define _GNU_SOURCE

#ifndef USER_H
#define USER_H
#define MAX_PRIV 5
#define MAX_USER 100
#define MAX_UNAME 16
#define MIN_UNAME 3
#define MAX_OUT 10 // Maximum number of output messages queued. prioritize all system messages, then all private messages, then public messages
#define CHANL_GEN "general"
#define CHANL_NICHE "niche"
#define SYS_MSG "System:\n\t"

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

struct user {
  char *uname;
  int fd;
  char *channel;
  char *req_from[MAX_PRIV];
  char *reqs[MAX_PRIV];
  int reqs_len;
  char *outbuffer;
  int out_len;
  struct user *next;
  struct user *prev;
};

void release(struct user *connection);
struct user *conn_create(const char *uname, int fd);
struct user *get_user(struct user *list, const int fd, const char *uname);
struct user *conn_insert(struct user **list, struct user *newConnection);
struct user *conn_remove(struct user **list, struct user *toRemove);
// void conn_fprint(FILE* file, struct user *list);
int conn_var_index(struct user *conn, char *from);
void update_buffers(struct user *conn, struct user *from, char *message[], const int len);
// int write_clients(struct user *conn);

#endif
