
#include "stream.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static void st_event_handler(struct poll_event *event, void *user_data);

struct read_callback_ctx {

  struct stream *stream;

  char *data;
  long long size;

  st_read_callback callback;
  void *user_data;
};

struct stream_callback_ctx {

  struct stream *stream;

  void (*callback) (void *);
  void *user_data;
};

static long long socket_read(int fd, char *buffer, long long size) {

	int n = read(fd, buffer, size);

	if (n == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -1;	
		}

		return -3;
	}

	return n;
}

static int st_receive_data(struct stream *s, long long *bytes_read) {

	char buffer[8192];
  long long n = 0;
  int t;

  *bytes_read = 0;

  while (1) {
    t = socket_read(s->fd, buffer, sizeof(buffer));

    if (t <= 0) {
      break;
    }

    n += t;
    array_catb(&s->buffer, buffer, t);
  }

  *bytes_read = n;

	return t;
}

static char *st_strnstr(char *haystack, char *needle, long long len) {

  long long i, needle_len = strlen(needle);

  for (i = 0; i + needle_len - 1 < len; ++i) {

    if (haystack[i] == *needle) {
      if (memcmp(haystack + i, needle, needle_len) == 0) {
        return haystack + i + needle_len;
      }
    }
  }

  return 0;
}

static int st_can_process_read_callback(struct stream *s, char **out_buffer, long long *out_size) {

  char *buffer = array_start(&s->buffer);
  long long len = array_length(&s->buffer, sizeof(char));

  if (len == 0) {
    return 0;
  }

  switch (s->condition_type) {

    case ST_CONDITION_NONE:
      break;

    case ST_CONDITION_UNTIL:

      {
        char *t = st_strnstr(buffer, s->condition.delimiter, len);

        if (t) {

          *out_buffer = buffer;
          *out_size = t - buffer;

          return 1;
        }
      }
      break;

    case ST_CONDITION_BYTES:

      if (len >= s->condition.bytes) {
        *out_buffer = buffer;
        *out_size = s->condition.bytes;

        return 1;
      }

      break;
  }

  return 0;
}

static void st_schedule_callback(struct stream *s, void (*callback)(void *), void *user_data) {
  s->pending_callbacks++;
  loop_schedule_callback(s->loop, callback, user_data);
} 

static void st_delegate_read_callback(void *user_data) {

  struct read_callback_ctx *ctx = (struct read_callback_ctx *)user_data;
  struct stream *stream = ctx->stream;

  stream->pending_callbacks--;

  ctx->callback(ctx->data, ctx->size, ctx->user_data);
  free(ctx);
}

static void st_delegate_stream_callback(void *user_data) {

  struct stream_callback_ctx *ctx = (struct stream_callback_ctx *)user_data;
  struct stream *stream = ctx->stream;

  stream->pending_callbacks--;

  ctx->callback(ctx->user_data);
  free(ctx);
}

static void st_schedule_stream_callback(struct stream *s, void (*callback)(void *), void *user_data) {

  if (callback == 0) {
    return;
  }

  struct stream_callback_ctx *ctx = malloc(sizeof(*ctx));

  ctx->stream = s;
  ctx->callback = callback;
  ctx->user_data = user_data;

  st_schedule_callback(s, st_delegate_stream_callback, ctx);
}

static void st_schedule_read_callback(struct stream *s, char *data, long long size) {

  struct read_callback_ctx *ctx = malloc(sizeof(*ctx));

  ctx->stream = s;
  ctx->data = data;
  ctx->size = size;
  ctx->callback = s->read_callback.callback;
  ctx->user_data = s->read_callback.user_data;

  s->read_callback.callback = 0;

  st_schedule_callback(s, st_delegate_read_callback, ctx);
}

static void st_schedule_write_callback(struct stream *s) {
  st_schedule_stream_callback(s, s->write_callback.callback, s->write_callback.user_data);
  s->write_callback.callback = 0;
}

static void st_schedule_close_callback(struct stream *s) {
  if (st_is_closed(s) && s->close_callback.callback && !s->pending_callbacks) {
    st_schedule_stream_callback(s, s->close_callback.callback, s->close_callback.user_data);
    s->close_callback.callback = 0;    
  }
}

static void st_add_io_state(struct stream *s, int event) {
  if (s->events == 0) {
    s->events = event;
    loop_add_handler(s->loop, s->fd, event, st_event_handler, s);
  } else if ((s->events & event) == 0) {
    s->events |= event;
    loop_modify_handler(s->loop, s->fd, s->events);
  }
}

static void st_try_inline_read(struct stream *s) {

  char *buffer; 
  long long buffer_len; 

  if (st_can_process_read_callback(s, &buffer, &buffer_len)) {
    printf("INLINE\n");
    st_schedule_read_callback(s, buffer, buffer_len);
  } else {
    long long n; 
    int retval = st_receive_data(s, &n);

    if (retval == 0 || retval == -3) {
      st_close(s);
    } else {

      if (n > 0) {
        if (st_can_process_read_callback(s, &buffer, &buffer_len)) {
          printf("INLINE\n");
          st_schedule_read_callback(s, buffer, buffer_len);
        }       
      } else {
        st_add_io_state(s, LOOP_READ);
      }
    }
  }
}

static int st_is_writing(struct stream *s) {
  return array_length(&s->out_buffer, sizeof(char)) > 0;
}

static int st_is_reading(struct stream *s) {
  return s->read_callback.callback != 0;
}

static void st_handle_read(struct stream *s) {

  long long n;
  int retval = st_receive_data(s, &n);

  if (retval == 0 || retval == -3) {
    st_close(s);
  } else {

    if (n > 0) {
      char *buffer; 
      long long buffer_len;

      if (st_can_process_read_callback(s, &buffer, &buffer_len)) {
        st_schedule_read_callback(s, buffer, buffer_len);
      }       
    }
  }
}

static void st_handle_write(struct stream *s) {
  
  int x = write(s->fd, array_start(&s->out_buffer), array_length(&s->out_buffer, sizeof(char)));

  if (x == -1) {
    return;
  }

  array_strip(&s->out_buffer, sizeof(char), x);

  if (array_length(&s->out_buffer, sizeof(char)) == 0) {
    st_schedule_write_callback(s);
  }

}

static void st_event_handler(struct poll_event *event, void *user_data) {

  struct stream *stream = (struct stream *)user_data;

  if (st_is_closed(stream)) {
    st_schedule_close_callback(stream);
    return;
  }

  if (event->events & LOOP_ERROR) {
    st_close(stream);
    return;
  }

  if (event->events & LOOP_READ) {
    st_handle_read(stream);
  }

  if (st_is_closed(stream)) {
    return;
  }

  if (event->events & LOOP_WRITE) {
    st_handle_write(stream);
  }

  if (st_is_closed(stream)) {
    return;
  }

  {
    int new_events = 0;

    new_events |= LOOP_READ * st_is_reading(stream);
    new_events |= LOOP_WRITE * st_is_writing(stream);

    if (new_events != stream->events) {
      if (new_events == 0) {
        //loop_remove_handler(stream->loop, stream->fd);
      } else {
        stream->events = new_events;
        loop_modify_handler(stream->loop, stream->fd, new_events);
      }
    }
  }
}

void st_read_until(struct stream *s, char *delimiter, st_read_callback cb, void *user_data) {

  s->condition_type = ST_CONDITION_UNTIL;
  s->condition.delimiter = delimiter;
  s->read_callback.callback = cb;
  s->read_callback.user_data = user_data;

  st_try_inline_read(s);
}

void st_read_bytes(struct stream *s, long long bytes, st_read_callback cb, void *user_data) {

  s->condition_type = ST_CONDITION_BYTES;
  s->condition.bytes = bytes;
  s->read_callback.callback = cb;
  s->read_callback.user_data = user_data;

  st_try_inline_read(s);
}

int st_is_closed(struct stream *s) {
  return s->fd == -1;
}

void st_close(struct stream *s) {

  if (!st_is_closed(s)) {

    array_free(&s->buffer);
    array_free(&s->out_buffer);

    loop_remove_handler(s->loop, s->fd);

    close(s->fd);
    s->fd = -1;
  }

  st_schedule_close_callback(s);
}

void st_set_close_callback(struct stream *s, st_close_callback cb, void *user_data) {

  s->close_callback.callback = cb;
  s->close_callback.user_data = user_data;
}

void st_set_write_callback(struct stream *s, st_write_callback cb, void *user_data) {

  s->write_callback.callback = cb;
  s->write_callback.user_data = user_data;
}

void st_writes(struct stream *stream, const char *s) {
  array_catb(&stream->out_buffer, (char *)s, strlen(s));
  st_add_io_state(stream, LOOP_WRITE);
}

void st_init(struct stream *s, struct loop *loop, int fd) {

  memset(s, 0, sizeof(*s));

  s->fd = fd;
  s->loop = loop;
}

