#ifndef __RING_H
#define __RING_H

struct ring_node {
	struct ring_node *next;
};

#define RING_INIT(r) {&(r)}

#define ring_mk_empty() ((struct ring_node *)0)

#define ring_is_empty(r) ((r) == ring_mk_empty())

#define ring_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

static inline struct ring_node *ring_head(struct ring_node *r) {
	return r->next;
}

static inline struct ring_node *ring_tail(struct ring_node *r) {

	struct ring_node *front = r->next;
	struct ring_node *front2 = front->next;

	if (r == front) {
		return (struct ring_node *)0;
	}

	r->next = front2;
	return r;
}

static inline struct ring_node *ring_concat(struct ring_node *l1, struct ring_node *l2) {

	if (ring_is_empty(l1)) {
		return l2;
	}

	if (ring_is_empty(l2)) {
		return l1;
	}

	{
		struct ring_node *t = l1->next;

		l1->next = l2->next;
		l2->next = t;
		return l2;
	}
}

static inline struct ring_node *ring_prepend(struct ring_node *l1, struct ring_node *l2) {
	return ring_concat(l2, l1);
}

static inline struct ring_node *ring_append(struct ring_node *l1, struct ring_node *l2) {
	return ring_concat(l1, l2);
}

#endif
