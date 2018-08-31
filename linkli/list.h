struct list_node {
	struct list_node *next, *prev;
};

typedef struct list_node *list_t;

#define list_node_init(_ln) \
	(_ln).next = &(_ln); \
	(_ln).prev = &(_ln)

#include <stdlib.h>
#define list_new_entry(_entry) \
	({ __typeof__(*_entry) *_new_entry = malloc(sizeof(__typeof__(*_entry))); \
	   list_node_init(_new_entry->ln); _new_entry; })

static inline void
list_insert(struct list_node *_new,
            struct list_node *prev, struct list_node *next)
{
	next->prev = _new;
	_new->next = next;
	_new->prev = prev;
	prev->next = _new;
}

static inline void
list_detach(struct list_node *prev, struct list_node *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline struct list_node *
list_remove(list_t *li, struct list_node *entry)
{
	list_detach(entry->prev, entry->next);
	if (entry == *li) {
		if (entry->next == entry)
			*li = NULL;
		else
			*li = entry->next;
	}

	return entry;
}

static inline int list_empty(list_t li)
{
	return (li == NULL);
}

static inline int list_size(list_t li)
{
	struct list_node *n = li;
	int cnt = 0;
	if (list_empty(li)) return 0;
	do {
		cnt += 1;
		n = n->next;
	} while (n != li);
	return cnt;
}

static inline void
list_append(list_t *li, struct list_node *_new)
{
	if (*li == NULL)
		*li = _new;
	else
		list_insert(_new, (*li)->prev, *li);
}

static inline void
list_insert_front(list_t *li, struct list_node *_new)
{
	list_append(li, _new);
	*li = _new;
}

typedef struct list_iterator {
	struct list_node *cur;
	list_t            li;
} *list_iter_t;

static inline list_iter_t
list_iterator(list_t li)
{
	struct list_iterator *iter;
	iter = malloc(sizeof(struct list_iterator));
	iter->cur = li;
	iter->li  = li;
	return iter;
}

static inline void
list_iter_free(list_iter_t iter)
{
	free(iter);
}

static inline int
list_iter_next(list_iter_t iter)
{
	if (iter->cur == NULL)
		return 0;

	iter->cur = iter->cur->next;

	if (iter->cur == iter->li)
		return 0;
	else
		return 1;
}

#include "common/util-macro.h"

#define list_entry(_entry, _node_addr) \
	container_of(_node_addr, __typeof__(*_entry), ln)

#define list_element(_entry, _node_addr) \
	(_node_addr == NULL) ? NULL : \
	(__typeof__(_entry))((uintptr_t)(_node_addr) - sizeof(__typeof__(*_entry)))

#define list_append_array(_list, _arr) \
	for (int i = 0; i < sizeof(_arr) / sizeof(_arr[0]); i++) { \
		struct { \
			__typeof__(_arr[0]) data; \
			struct list_node ln; \
		} __attribute__((packed)) *n; \
		n = list_new_entry(n); \
		n->data = _arr[i]; \
		list_append(&(_list), &n->ln); \
	} do {} while (0)

#define list_free_entry(_type, _iter) \
	{ \
		int sz = list_size((_iter)->cur); \
		list_detach((_iter)->cur->prev, (_iter)->cur->next); \
		_type *obj = list_element(obj, (_iter)->cur); \
		free(obj); \
		if (sz - 1 == 0) { \
			li = NULL; \
			break; \
		} \
	} do {} while (0)
