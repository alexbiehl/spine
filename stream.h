#ifndef __STREAM_H
#define __STREAM_H

#include "array.h"
#include "loop.h"

typedef void (*st_read_callback) (char *, long long, void *);

typedef void (*st_close_callback) (void *);

typedef void (*st_write_callback) (void *);

struct stream {

  int fd;

  struct loop *loop;  
  
  array buffer;
  array out_buffer;

  int events;
  int pending_callbacks;

  enum {
    ST_CONDITION_NONE,
    ST_CONDITION_UNTIL,
    ST_CONDITION_BYTES
  } condition_type;

  union { 
    char *delimiter;
    long long bytes;
  } condition;

  struct {
    st_read_callback callback;
    void *user_data;
  } read_callback;

  struct {
    st_write_callback callback;
    void *user_data;
  } write_callback;

  struct {
    st_close_callback callback;
    void *user_data;
  } close_callback;
};

void st_init(struct stream *s, struct loop *loop, int fd);

void st_set_write_callback(struct stream *s, st_write_callback cb, void *user_data);
void st_set_close_callback(struct stream *s, st_close_callback cb, void *user_data);

void st_read_until(struct stream *s, char *delimiter, st_read_callback cb, void *user_data);
void st_read_bytes(struct stream *s, long long bytes, st_read_callback cb, void *user_data);

void st_writes(struct stream *stream, const char *s);
void st_writes_free(struct stream *stream, const char *s);
void st_writef(struct stream *stream, int fd, long long offset, long long length);
void st_writef_close(struct stream *stream, int fd, long long offset, long long length);

void st_close(struct stream *s);
int st_is_closed(struct stream *s);


#endif
