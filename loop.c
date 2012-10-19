
#include "loop.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct scheduled_callback {
	loop_callback callback;
	void *user_data;

	struct ring_node node;
};

struct handler {
	loop_handler handler_fun;
	void *user_data;
	int events;
};

int loop_init(struct loop *l) {

	memset(l, 0, sizeof(*l));

	l->scheduled_callbacks = ring_mk_empty();

	return poll_init(&l->poll);
}

static void loop_run_callbacks(struct loop *l) {

	while (!ring_is_empty(l->scheduled_callbacks)) {

		struct scheduled_callback *callback = ring_entry(
			ring_head(l->scheduled_callbacks), struct scheduled_callback, node);

		l->scheduled_callbacks = ring_tail(l->scheduled_callbacks);

		callback->callback(callback->user_data);
		free(callback);
	}
}

void loop_schedule_callback(struct loop *l, loop_callback callback, void *user_data) {

	struct scheduled_callback *cb = malloc(sizeof(*cb));

	cb->callback = callback;
	cb->user_data = user_data;
	cb->node.next = &cb->node;

	l->scheduled_callbacks = ring_append(l->scheduled_callbacks, &cb->node);
}

int loop_add_handler(struct loop *l, int fd, int events, loop_handler handler, void *user_data) {

	int retval = poll_register(&l->poll, fd, events);

	if (retval) {
		struct handler *h = array_allocate(&l->handlers, sizeof(*h), fd);

		h->handler_fun = handler;
		h->user_data = user_data;
		h->events = events;
	}

	return retval;
}

int loop_modify_handler(struct loop *l, int fd, int events) {

	struct handler *h = array_get(&l->handlers, sizeof(*h), fd);

	if (h->events != events) {

    poll_remove_all(&l->poll, fd);
		poll_register(&l->poll, fd, events);

		h->events = events;

		return 1;
	}

	return 1;
}

int loop_remove_handler(struct loop *l, int fd) {
	struct handler *h = array_get(&l->handlers, sizeof(*h), fd);
	
	return poll_remove(&l->poll, fd, h->events);
}

void loop_start(struct loop *l) {

	while (1) {

		loop_run_callbacks(l);

		{
			struct poll_event events[16];

			int i, n = poll_wait(&l->poll, 0, events, sizeof(events)/sizeof(events[0]));

			for (i = 0; i < n; ++i) {

				struct handler *h = array_get(&l->handlers, sizeof(*h), events[i].fd);

				h->handler_fun(events + i, h->user_data);
			}
		}
	}
}
