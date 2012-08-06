#ifndef __LOOP_H
#define __LOOP_H

#include "ring.h"
#include "poll.h"
#include "array.h"

typedef void (*loop_callback) (void *);

typedef void (*loop_handler)(struct poll_event *, void *);

#define LOOP_READ POLL_READ
#define LOOP_WRITE POLL_WRITE
#define LOOP_ERROR POLL_ERROR

struct loop {
  array handlers;
  struct poll poll;
  struct ring_node *scheduled_callbacks;
};

int loop_init(struct loop *l);

int loop_add_handler(struct loop *l, int fd, int events, loop_handler handler, void *user_data);

int loop_modify_handler(struct loop *l, int fd, int events);

int loop_remove_handler(struct loop *l, int fd);

void loop_start(struct loop *l);

void loop_schedule_callback(struct loop *l, loop_callback callback, void *user_data);

#endif