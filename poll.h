#ifndef __POLL_H
#define __POLL_H

#define POLL_READ 1
#define POLL_WRITE 2
#define POLL_ERROR 4

struct poll {
	int fd;
};

struct poll_event {
  int fd;
  int events;
};

int poll_init(struct poll *poll);

int poll_register(struct poll *poll, int fd, int events);

int poll_modify(struct poll *poll, int fd, int events);

int poll_remove_all(struct poll *poll, int fd);

int poll_remove(struct poll *poll, int fd, int events);

int poll_wait(struct poll *poll, long long milliseconds, struct poll_event *events, long long max_events);

#endif
