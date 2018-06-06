#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "strmap.h"
#include "config.h"

typedef struct strmap_entry entry_t;

strmap_t strmap_new()
{
	struct strmap *map = malloc(sizeof(struct strmap));
	map->length = 0;
	map->n_entries = INIT_ENTRIES;
	map->entries = malloc(sizeof(entry_t) * INIT_ENTRIES);
	map->dat = datrie_new();

	return map;
}

void strmap_free(strmap_t map)
{
	for (int i = 0; i < map->length; i++) {
		free(map->entries[i].keystr);
	}

	free(map->entries);
	datrie_free(map->dat);
	free(map);
}

void **strmap_val_ptr(strmap_t map, char *key)
{
	struct strmap_entry *ent;
	datrie_state_t key_idx;
	key_idx = datrie_lookup(&map->dat, key);

	if (key_idx != 0) {
		ent = &map->entries[key_idx - 1];
		return &ent->value;
	}

	key_idx = datrie_insert(&map->dat, key);
	assert(map->length + 1 == key_idx);

	if (key_idx >= map->n_entries) {
		map->n_entries *= 2;
#ifdef DEBUG_STRMAP
		printf("resizing entry array to %u \n", map->n_entries);
#endif
		size_t alloc_sz = sizeof(entry_t) * map->n_entries;
		map->entries = realloc(map->entries, alloc_sz);
	}

	ent = &map->entries[map->length ++];
	ent->keystr = strdup(key);
	ent->value = NULL;

	return &ent->value;
}

struct strmap *strmap_va_list(int n, ...)
{
	struct strmap *m = strmap_new();

	va_list valist;
	va_start(valist, n);
	for (int i = 0; i < n / 2; i++) {
		char *key = va_arg(valist, char*);
		void *val = va_arg(valist, void*);
		*strmap_val_ptr(m, key) = val;
	}

	return m;
}

struct strmap_iterator strmap_iterator(strmap_t m)
{
	return (struct strmap_iterator) {0, m->entries};
}

int strmap_empty(strmap_t m)
{
	return (m == NULL);
}

int strmap_iter_next(strmap_t m, struct strmap_iterator *iter)
{
	iter->cur += 1;
	iter->idx += 1;
	if (iter->idx > m->length - 1)
		return 0;
	else
		return 1;
}
