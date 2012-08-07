
#include "poll.h"

#include <sys/time.h>
#include <sys/event.h>

#include <assert.h>

int poll_init(struct poll *poll) {

	poll->fd = kqueue();

	return poll->fd != -1;
}

int poll_register(struct poll *poll, int fd, int events) {

	if ((events & (POLL_READ | POLL_WRITE)) == (POLL_READ | POLL_WRITE)) {
		return poll_register(poll, fd, POLL_READ) && poll_register(poll, fd, POLL_WRITE);
	} else {

		struct kevent event;

		int filter = 0;

		if (events & POLL_READ) {
			filter = EVFILT_READ;
		} else if (events & POLL_WRITE) {
			filter = EVFILT_WRITE;
		}

		EV_SET(&event, fd, filter, EV_ADD | EV_ENABLE, 0, 0, 0);

		int x = kevent(poll->fd, &event, 1, 0, 0, 0) != -1;

		assert(x);

		return x;
	}
}

int poll_modify(struct poll *poll, int fd, int events) {
	return poll_remove_all(poll, fd) && poll_register(poll, fd, events);
}

int poll_remove_all(struct poll *poll, int fd) {
	return poll_remove(poll, fd, POLL_READ | POLL_WRITE);
}

int poll_remove(struct poll *poll, int fd, int events) {

	if ((events & (POLL_READ | POLL_WRITE)) == (POLL_READ | POLL_WRITE)) {
		return poll_remove(poll, fd, POLL_READ) && poll_remove(poll, fd, POLL_WRITE);
	} else {

		struct kevent event;
		int filter = 0;

		if (events & POLL_READ) {
			filter = EVFILT_READ;
		} else if (events & POLL_WRITE) {
			filter = EVFILT_WRITE;
		}	

		EV_SET(&event, fd, filter, EV_DELETE, 0, 0, 0);

		int x = kevent(poll->fd, &event, 1, 0, 0, 0) != -1;

		assert(x);

		return x;
	}
}

int poll_wait(struct poll *poll, long long milliseconds, struct poll_event *events, long long max_events) {

	struct timespec ts;

	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = milliseconds % 1000;

	{
		struct kevent kevents[max_events];

		int n = kevent(poll->fd, 0, 0, kevents, max_events, milliseconds ? &ts : 0);

		assert(n != -1);

		if (n != -1) {

			int i;

			for (i = 0; i < n; ++i) {

				if (kevents[i].filter == EVFILT_READ) {
					events[i].events = POLL_READ;
				} else if (kevents[i].filter == EVFILT_WRITE) {
					events[i].events = POLL_WRITE;
				} else if (kevents[i].flags == EV_ERROR) {
					events[i].events = POLL_ERROR;
				}

				events[i].fd = kevents[i].ident;
			}
		}

		return n;
	}
}
