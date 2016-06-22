#pragma once

/*
 * A skip-list variant, it uses fixed jump-span length for each level, but
 * jump-span can be specified at runtime. Also, when jump() is performed,
 * skippy will not go back on a top level, but instead trying to forward
 * (monotone key change) from its current position. To enable going back on
 * a top level, define SKIPPY_ALWAYS_TOP_LEVEL.
 *
 * Wei Zhong
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "list/list.h"

#define SKIPPY_SKIP_LEVELS 2
#define SKIPPY_TOTAL_LEVELS (SKIPPY_SKIP_LEVELS + 1)

#define DEFAULT_SKIPPY_SPANS 3
#define MAX_SKIPPY_SPANS     UINT_MAX

#define skippy_foreach(_cur, _save, _sk, _level) \
	_cur = (_sk)->head[_level]; \
	if (_cur) \
	  for (_save = _cur->next[_level]; \
	     _cur != NULL; \
	     _cur = _save, _save = (_cur) ? _cur->next[_level] : NULL)

#define skippy_free(_sk, _type, _member) { \
		struct skippy_node *cur, *save; \
		_type* p; \
		skippy_foreach(cur, save, _sk, 0) { \
			p = MEMBER_2_STRUCT(cur, _type, _member); \
			free(p); \
		} \
	} do {} while(0)

struct skippy_node {
	struct skippy_node *next[SKIPPY_TOTAL_LEVELS];
	uint32_t            key;
};

struct skippy {
	uint32_t            n_spans;
	uint32_t            n_nodes[SKIPPY_TOTAL_LEVELS];
	struct skippy_node *head[SKIPPY_TOTAL_LEVELS];
	struct skippy_node *tail[SKIPPY_TOTAL_LEVELS];
};

static __inline void
skippy_init(struct skippy *sk, uint32_t n_spans)
{
	sk->n_spans = n_spans;
	memset(sk->n_nodes, 0, sizeof(uint32_t) * SKIPPY_TOTAL_LEVELS);
	memset(sk->head, 0, sizeof(void*) * SKIPPY_TOTAL_LEVELS);
	memset(sk->tail, 0, sizeof(void*) * SKIPPY_TOTAL_LEVELS);
}

/* skippy_node_jump() function jumps to the furthest possible node
 * that has a key less or equal to `target'. Return the same `sn'
 * if nowhere can go. */
static __inline struct skippy_node*
skippy_node_jump(struct skippy_node *sn, uint32_t target)
{
	int level = SKIPPY_TOTAL_LEVELS - 1;
	struct skippy_node *forward, *upward;
	while (1) {

		forward = sn->next[level];
		upward = sn->next[level + 1];

		if (upward && level + 1 < SKIPPY_TOTAL_LEVELS &&
		    upward->key <= target) {
			/* first see if we can safely go higher */
			sn = upward;
			level ++;
		} else if (forward && forward->key <= target) {
			/* or can we safely step forward? */
			sn = forward;
		} else {
			/* check if we are at the bottom level */
			if (level > 0)
				/* go downward */
				level --;
			else
				break;
		}

#ifdef DEBUG_SKIPPY
		printf("go to key=%u @ level %d\n", sn->key, level);
#endif
	}

	return sn;
}

static __inline void
skippy_node_init(struct skippy_node * sn, uint32_t key)
{
	memset(sn->next, 0, sizeof(void*) * SKIPPY_TOTAL_LEVELS);
	sn->key = key;
}

static __inline bool
_skippy_append_level(struct skippy *sk, struct skippy_node *sn, int level)
{
	uint32_t n_nodes;
	if (level == 0)
		n_nodes = 1;
	else
		n_nodes = sk->n_nodes[level - 1];

	if (n_nodes % sk->n_spans == 1 /* this is one so that we make every
	    level have the first skippy element as its head. */) {
		if (sk->head[level] == NULL) {
			sk->head[level] = sn;
			sk->tail[level] = sn;
		} else {
			sk->tail[level]->next[level] = sn;
			sk->tail[level] = sn;
		}

		sk->n_nodes[level] ++;
		return 1;
	} else {
		return 0;
	}
}

static __inline void
skippy_append(struct skippy *sk, struct skippy_node *sn)
{
	int i;
	for (i = 0; i < SKIPPY_TOTAL_LEVELS; i++)
		if (!_skippy_append_level(sk, sn, i))
			break;

#ifdef SKIPPY_ALWAYS_TOP_LEVEL
#warning "always go from top level"
	/* enable the ability to go back some keys
	 * but search from top level */
	for (j = i; j < SKIPPY_TOTAL_LEVELS; j++)
		sn->next[j] = sk->tail[j];
#endif
}

static __inline void
skippy_print(struct skippy *sk)
{
	int i = SKIPPY_TOTAL_LEVELS - 1;
	struct skippy_node *cur, *save;
	printf("skippy heads:\n");
	while (1) {
		printf("level %d (%u nodes): ", i, sk->n_nodes[i]);
		skippy_foreach(cur, save, sk, i) {
			printf("-%u", cur->key);
		}

		if (sk->tail[i])
			printf(" (tail=%u)", sk->tail[i]->key);
		printf("\n");

		if (i == 0)
			break;
		else
			i--;
	}
}

static __inline void
skippy_node_print(struct skippy_node *sn)
{
	int i = SKIPPY_TOTAL_LEVELS - 1;
	struct skippy_node *next;
	printf("node %u:\n", sn->key);
	while (1) {
		next = sn->next[i];
		printf("next -> ");
		if (next)
			printf("%u\n", next->key);
		else
			printf("NULL\n");

		if (i == 0)
			break;
		else
			i--;
	}
}
