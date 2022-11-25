#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef USER_H
#define USER_H
#define MAX_PRIV 2
#define MAX_USER 100
#define MAX_UNAME 16
#define MIN_UNAME 3
#define MAX_OUT 10 // Maximum number of output messages queued. prioritize all system messages, then all private messages, then public messages
#define CHANL_GEN "general"
#define CHANL_NICHE "niche"
#define RANK_SYS 0
#define RANK_PRIV 1 // priorities for output messages
#define RANK_PUB 2
#define SYS_MSG "System:\n    "



#include "utils.h"
#include "commands.h"

struct user {
  char *uname;
  int fd;
  char *channel;
  
  char *help_buffer; // help message is variable length (based on commands), so must be dynamically allocated
  
  int need_help;    // flag to indicate whether or not to send the help message. needed for the implementation of writing to the client
  char *reqs_from[MAX_PRIV]; // list of users who have currently open private channels with this user
  int reqs_len;
  int reqs_notify; // 0 if user doesn't want to be notified when someone opens a private channel with them (default 1)
  
  char *outbuffer[MAX_OUT]; // output buffer for messages to be sent to the client
  int out_rank[MAX_OUT]; // 0 = sys, 1 = priv, 2 = pub
  int out_len; // number of messages in the output buffer
  
  struct user *next;
  struct user *prev;
};

void release(struct user *connList, struct user *connection);
struct user *conn_create(const char *uname, int fd);
struct user *get_user(struct user *list, const int fd, const char *uname);
struct user *conn_insert(struct user **list, struct user *newConnection);
struct user *conn_remove(struct user **list, struct user *toRemove);
struct user *remove_buffer_i(struct user *conn, int index); // remove a message from a single client's output buffer at index
struct user *remove_buffer_rank(struct user *conn, int rank); // remove all messages of given rank from a single client's output buffer
struct user *empty_buffer(struct user *conn); // remove all messages from a single client's output buffer
struct user *pack_buffers(struct user *connList); // pack all messages from all clients into a single buffer
struct user *insert_buffer(struct user *conn, char *message, const int rank); // insert a message into a single client's output buffer
// void conn_fprint(FILE* file, struct user *list);
int req_remove(struct user *from, struct user *to);
int DM_notify(struct user *connList, struct user *from, struct user *to);
int get_rank_i(struct user *conn); // get the index of the lowest rank message in the output buffer
void update_buffers(struct user *conn, struct user *from, char *message[], const int len); // update all client's buffers
// int write_clients(struct user *conn);

#endif
