#ifndef __ARRAY_H
#define __ARRAY_H

#include <stdlib.h>
#include <string.h>

struct array {
	void *data;
	long long allocated;
	long long initialized;
};

typedef struct array array;

static inline void *array_allocate(struct array *a, long long member_size, long long n) {

	long long wanted = (n + 1) * member_size;

	if (wanted < 0) {
		return 0;
	}

	if (wanted > a->initialized) {

		if (wanted >= a->allocated) {

			if (member_size < 8) {
				wanted = (wanted + 127) & (-128);
			} else {
				wanted = (wanted + 4095) & (-4096);
			}

			{
				void *buffer = realloc(a->data, wanted);

				if (!buffer) {
					return 0;
				}

				a->data = buffer;
				a->allocated = wanted;
	 	  }
		}

		a->initialized = (n + 1) * member_size;
	}

  return a->data + n * member_size;
}

static inline void *array_get(struct array *a, long long member_size, long long n) {

/*  long long wanted = n * member_size;

  if (wanted < 0) {
    return 0;
  }*/

  return a->data + n * member_size;
}

static inline long long array_length(struct array *a, long long member_size) {

  return a->initialized / member_size;
}

static inline void *array_start(struct array *a) {
  return a->data;
}

static inline void array_free(struct array *a) {

  a->allocated = a->initialized = 0;

  free(a->data);
}

static inline void array_catb(struct array *a, char *buffer, long long n) {

  long long l = a->initialized;

  array_allocate(a, sizeof(char), l + n - 1);

  memcpy(a->data + l, buffer, n);
}

static inline void array_strip(struct array *a, long long member_size, long long n) {

  char *b = a->data;

  memcpy(b, b + member_size * n, a->initialized - member_size * n);
  a->initialized -= member_size * n;
  
}

#endif
