
#include "array.h"

void *array_allocate(struct array *a, long long member_size, long long n) {

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

  return (char *)a->data + n * member_size;
}

void array_free(struct array *a) {
  free(a->data);

  a->allocated = a->initialized = 0;
  a->data = 0;
}
