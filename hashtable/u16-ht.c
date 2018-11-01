#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "u16-ht.h"

const int rehash_sz[][2] = {
	/* size => load factor */
#ifdef DEBUG_U16_HASHTABLE
	{8,  6},
	{12,  8},
	{15,  10},
	{16,  12},
#endif
	{32, 24},
	{64, 48},
	{128, 96},
	{256, 192},
	{512, 384},
	{1024, 768},
	{2048, 1536},
	{4096, 3072},
	{8192, 6144},
	{16384, 12288},
	{32768, 24576},
	{65536, 49152}
};

#define TOTAL_SZ_LEVELS (sizeof(rehash_sz) / sizeof(rehash_sz[0]))

void u16_ht_reset(struct u16_ht *ht, int sz_level)
{
	ht->sz = rehash_sz[sz_level][0];
	ht->len = 0;
	ht->load_limit = rehash_sz[sz_level][1];
	ht->sz_level = sz_level;
	memset(ht->table, 0, ht->sz * sizeof(struct u16_ht_entry));
}

struct u16_ht u16_ht_new(int sz_level)
{
	struct u16_ht ht;
	int sz = rehash_sz[sz_level][0];
	ht.table = calloc(sz, sizeof(struct u16_ht_entry));
	u16_ht_reset(&ht, sz_level);
	return ht;
}

void u16_ht_free(struct u16_ht *ht)
{
	free(ht->table);
	memset(ht, 0, sizeof(struct u16_ht));
}

static __inline__ int probing(int key, int i)
{
	/* in a form of "a * key + b", make sure sz
	 * and a are co-prime in order to generate
	 * a complete circle in mod sz. */
	return key + i;
}

static __inline__ int keyhash(int key, int sz)
{
	return key % sz;
}

int u16_ht_lookup(struct u16_ht *ht, int key)
{
	int sz = ht->sz;
	struct u16_ht_entry *table = ht->table;

	for (int i = 0; i < sz; i++) {
		int pos = (keyhash(key, sz) + probing(key, i)) % sz;
		if (table[pos].occupied) {
			if (table[pos].key == key)
				return table[pos].val;
			else
				continue;
		} else {
			return -1;
		}
	}

	return -1;
}

static void u16_ht_rehash(struct u16_ht*);

void
u16_ht_insert(struct u16_ht *ht, int key, int val)
{
	int sz = ht->sz;
	struct u16_ht_entry *table = ht->table;

	for (int i = 0; i < sz; i++) {
		int pos = (keyhash(key, sz) + probing(key, i)) % sz;
		if (0 == table[pos].occupied) {
			table[pos].occupied = 1;
			table[pos].key = key;
			table[pos].val = val;
			break;
		}
	}

	ht->len ++;
	if (ht->len > ht->load_limit)
		u16_ht_rehash(ht);
}

static void u16_ht_rehash(struct u16_ht *ht)
{
	assert(ht->sz_level != TOTAL_SZ_LEVELS);
	int sz_level = ht->sz_level + 1;
	struct u16_ht tmp_ht = u16_ht_new(sz_level);

#ifdef DEBUG_U16_HASHTABLE
	printf("rehashing...\n");
#endif

	for (int i = 0; i < ht->sz; i++)
		if (ht->table[i].occupied)
			u16_ht_insert(&tmp_ht, ht->table[i].key, ht->table[i].val);

	u16_ht_free(ht);
	*ht = tmp_ht;
}

void u16_ht_print(struct u16_ht *ht)
{
	for (int i = 0; i < ht->sz; i++)
		if (ht->table[i].occupied)
			printf("[%u] %u: %u ", i, ht->table[i].key, ht->table[i].val);

	printf(" (%d/%d/%d:%d)\n", ht->len, ht->load_limit,
	       ht->sz, ht->sz_level);
	return;
}
