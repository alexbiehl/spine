#include "poll.h"

#include <sys/epoll.h>

int poll_init(struct poll *poll) {

  poll->fd = epoll_create(1024);

  return poll->fd != -1;
}

static int epoll_op(int efd, int op, int fd, int events) {

  struct epoll_event e;

  e.events = 0;
  e.events |= EPOLLIN * ((events & POLL_READ) != 0);
  e.events |= EPOLLOUT * ((events & POLL_WRITE) != 0);
  e.data.fd = fd;

  return epoll_ctl(efd, op, fd, &e) != -1;
}


int poll_register(struct poll *poll, int fd, int events) {
  return epoll_op(poll->fd, EPOLL_CTL_ADD, fd, events);
}

int poll_modify(struct poll *poll, int fd, int events) {
  return epoll_op(poll->fd, EPOLL_CTL_MOD, fd, events);
}

int poll_remove_all(struct poll *poll, int fd) {
  return epoll_op(poll->fd, EPOLL_CTL_DEL, fd, 0);
}

int poll_remove(struct poll *poll, int fd, int events) {

  if (events == (POLL_READ | POLL_WRITE)) {
    return poll_remove_all(poll, fd);
  } else {
    return poll_modify(poll, fd, events);
  }
}

int poll_wait(struct poll *poll, long long milliseconds, struct poll_event *events, long long max_events) {

  struct epoll_event ee[max_events];

  int n = epoll_wait(poll->fd, ee, max_events, milliseconds);

  if (n != -1) {

    int i;

    for (i = 0; i < n; ++i) {

      int e = 0;

      e |= POLL_ERROR * ((ee[i].events & EPOLLERR) != 0);
      e |= POLL_READ * ((ee[i].events & EPOLLIN) != 0);
      e |= POLL_WRITE * ((ee[i].events & EPOLLOUT) != 0);

      events[i].events = e;
      events[i].fd = ee[i].data.fd;
    }
  }

  return n;
}
