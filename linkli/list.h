#pragma once

struct li_node {
	struct li_node *next, *prev;
};

typedef struct li_node *linkli_t;

#define li_node_init(_ln) \
	(_ln).next = &(_ln); \
	(_ln).prev = &(_ln)

#include <stdlib.h>
#define li_new_entry(_entry, _member) \
	({ __typeof__(*_entry) *_new_entry = malloc(sizeof(__typeof__(*_entry))); \
	   li_node_init(_new_entry->_member); _new_entry; })

static inline void
li_insert(struct li_node *_new,
          struct li_node *prev, struct li_node *next)
{
	next->prev = _new;
	_new->next = next;
	_new->prev = prev;
	prev->next = _new;
}

static inline void
li_detach(struct li_node *prev, struct li_node *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline struct li_node *
li_remove(linkli_t *li, struct li_node *entry)
{
	li_detach(entry->prev, entry->next);
	if (entry == *li) {
		if (entry->next == entry)
			*li = NULL;
		else
			*li = entry->next;
	}

	return entry;
}

static inline int li_empty(linkli_t li)
{
	return (li == NULL);
}

static inline int li_size(linkli_t li)
{
	struct li_node *n = li;
	int cnt = 0;
	if (li_empty(li)) return 0;
	do {
		cnt += 1;
		n = n->next;
	} while (n != li);
	return cnt;
}

static inline void
li_append(linkli_t *li, struct li_node *_new)
{
	if (*li == NULL)
		*li = _new;
	else
		li_insert(_new, (*li)->prev, *li);
}

static inline void
li_insert_front(linkli_t *li, struct li_node *_new)
{
	li_append(li, _new);
	*li = _new;
}

typedef struct li_iterator {
	struct li_node *cur;
	linkli_t        li;
} *li_iter_t;

static inline li_iter_t
li_iterator(linkli_t li)
{
	struct li_iterator *iter;
	iter = malloc(sizeof(struct li_iterator));
	iter->cur = li;
	iter->li  = li;
	return iter;
}

static inline void
li_iter_free(li_iter_t iter)
{
	free(iter);
}

static inline int
li_iter_next(li_iter_t iter)
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

#define li_entry(_entry, _node_addr, _member) \
	container_of(_node_addr, __typeof__(*_entry), _member)

/* an utility function to append list from array w/o defining list node */
#define li_append_array(_list, _arr) \
	for (int i = 0; i < sizeof(_arr) / sizeof(_arr[0]); i++) { \
		struct { \
			__typeof__(_arr[0]) data; \
			struct li_node ln; \
		} *n; \
		n = li_new_entry(n, ln); \
		n->data = _arr[i]; \
		li_append(&(_list), &n->ln); \
	} do {} while (0)

/* li_element should only be used in list created from li_append_array() */
#define li_element(_entry, _node_addr) \
	(_node_addr == NULL) ? NULL : \
	(__typeof__(_entry))((uintptr_t)(_node_addr) - sizeof(__typeof__(*_entry)))
