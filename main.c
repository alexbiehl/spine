
#include "ring.h"
#include "loop.h"
#include "stream.h"
#include "net.h"

#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>

static int counter = 0;

static int listener = 0;
static struct loop loop;

void close_callback(void *user_data) {
	free(user_data);

	counter--;

//	printf("%u\n", counter);
}

void header_complete(char *header, long long header_size, void *user_data) {

	(void)header;
	(void)header_size;

	//printf("%s", header);

	st_writes(user_data, "HTTP/1.1 200 Ok\r\nContent-Length: 3\r\n\r\nabc");
}

void accept_handler(struct poll_event *event, void *user_data) {

	(void)user_data;

	if (event->events & POLL_READ) {

		while (1) {

			int n = accept(listener, 0, 0);

			if (n == -1) {
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					
					printf("%s\n", strerror(errno));
					assert(0);
				}

				break;
			}

			{
				struct stream *s = malloc(sizeof(*s));

				st_init(s, &loop, n);

				set_non_block(n);

				counter++;

				st_set_write_callback(s, (loop_callback)st_close, s);
				st_set_close_callback(s, close_callback, s);
				st_read_until(s, "\r\n\r\n", header_complete, s);
			}
		}
	}
}


int main(void) {

	signal(SIGPIPE, SIG_IGN);

	listener = tcp_create_listener(0, "8080", 256);

	assert(listener != -1);


	set_non_block(listener);

	loop_init(&loop);

	loop_add_handler(&loop, listener, POLL_READ, accept_handler, 0);

	loop_start(&loop);

}
