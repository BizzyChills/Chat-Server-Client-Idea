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
#define RANK_SYS 0
#define RANK_PRIV 1 // priorities for output messages
#define RANK_PUB 2
#define SYS_MSG "System:\n\t"


#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "commands.h"

struct user {
  char *uname;
  int fd;
  char *channel;
  char *req_from[MAX_PRIV];
  char *reqs[MAX_PRIV];
  int reqs_len;
  char *outbuffer[MAX_OUT];
  int out_rank[MAX_OUT]; // 0 = sys, 1 = priv, 2 = pub
  int out_len;
  struct user *next;
  struct user *prev;
};

void release(struct user *connection);
struct user *conn_create(const char *uname, int fd);
struct user *get_user(struct user *list, const int fd, const char *uname);
struct user *conn_insert(struct user **list, struct user *newConnection);
struct user *conn_remove(struct user **list, struct user *toRemove);
struct user *remove_buffer(struct user *conn, int index); // remove a message from a single client's output buffer
struct user *empty_buffer(struct user *conn); // remove all messages from a single client's output buffer
struct user *pack_buffers(struct user *connList); // pack all messages from all clients into a single buffer
struct user *insert_buffer(struct user *conn, char *message, const int rank); // insert a message into a single client's output buffer
// void conn_fprint(FILE* file, struct user *list);
int get_reqt(struct user *conn, char *from);
int get_rank_i(struct user *conn); // get the index of the lowest rank message in the output buffer
void update_buffers(struct user *conn, struct user *from, char *message[], const int len); // update all client's buffers
// int write_clients(struct user *conn);

#endif
