#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "strmap.h"

#define INIT_ENTRIES 4

typedef struct strmap_entry entry_t;
typedef struct strmap strmap_t;

strmap_t *strmap_new()
{
	struct strmap *map = malloc(sizeof(struct strmap));
	map->length = 0;
	map->n_entries = INIT_ENTRIES;
	map->entries = malloc(sizeof(entry_t) * INIT_ENTRIES);
	map->dat = datrie_new();

	return map;
}

void strmap_free(strmap_t *map)
{
	for (int i = 0; i < map->n_entries; i++) {
		free(map->entries[i].keystr);
	}

	free(map->entries);
	datrie_free(map->dat);
	free(map);
}

void **strmap_val_ptr(strmap_t *map, char *key)
{
	struct strmap_entry *ent;
	datrie_state_t key_idx;
	key_idx = datrie_lookup(&map->dat, key);
	
	printf("strmap[%s] = %u \n", key, key_idx);

	if (key_idx != 0) {
		ent = &map->entries[key_idx - 1];
		return &ent->value;
	}

	key_idx = datrie_insert(&map->dat, key);
	assert(map->length + 1 == key_idx);
	
	printf("alloc strmap[%s] = %u \n", key, key_idx);

	if (key_idx >= map->n_entries) {
		map->n_entries *= 2;
		size_t alloc_sz = sizeof(entry_t) * map->n_entries;
		map->entries = realloc(map->entries, alloc_sz);
		;
	}

	ent = &map->entries[map->length ++];
	ent->keystr = strdup(key);
	ent->value = NULL;

	return &ent->value;
}
