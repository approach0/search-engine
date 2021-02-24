#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "float-ht.h"

static const int rehash_sz[][2] = {
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

void float_ht_reset(struct float_ht *ht, int sz_level)
{
	ht->sz = rehash_sz[sz_level][0];
	ht->len = 0;
	ht->load_limit = rehash_sz[sz_level][1];
	ht->sz_level = sz_level;
	memset(ht->table, 0, ht->sz * sizeof(struct float_ht_entry));
}

struct float_ht float_ht_new(int sz_level)
{
	struct float_ht ht;
	int sz = rehash_sz[sz_level][0];
	ht.table = calloc(sz, sizeof(struct float_ht_entry));
	float_ht_reset(&ht, sz_level);
	return ht;
}

void float_ht_free(struct float_ht *ht)
{
	free(ht->table);
	memset(ht, 0, sizeof(struct float_ht));
}

static __inline__ int probing(int hash, int i)
{
	/* in a form of "a * key + b", make sure sz
	 * and a are co-prime in order to generate
	 * a complete circle in mod sz. */
	return hash + i;
}

float float_ht_lookup(struct float_ht *ht, int key)
{
	int sz = ht->sz;
	struct float_ht_entry *table = ht->table;

	for (int i = 0; i < sz; i++) {
		int pos = probing(key, i) % sz;
		if (table[pos].occupied) {
			if (table[pos].key == key)
				return table[pos].val;
			else
				continue;
		} else {
			return -1.f;
		}
	}

	return -1.f;
}

static void float_ht_rehash(struct float_ht*);

void
float_ht_update(struct float_ht *ht, int key, float val)
{
	int sz = ht->sz;
	struct float_ht_entry *table = ht->table;

	for (int i = 0; i < sz; i++) {
		int pos = probing(key, i) % sz;
		if (0 == table[pos].occupied) {
			table[pos].occupied = 1;
			table[pos].key = key;
			table[pos].val = val;
			ht->len ++;
			break;
		} else if (table[pos].key == key) {
			table[pos].val = val;
			break;
		}
	}

	if (ht->len > ht->load_limit)
		float_ht_rehash(ht);
}

float float_ht_incr(struct float_ht *ht, int key, float incr)
{
	int sz = ht->sz;
	struct float_ht_entry *table = ht->table;
	float ret = -1.f;

	for (int i = 0; i < sz; i++) {
		int pos = probing(key, i) % sz;
		if (0 == table[pos].occupied) {
			table[pos].occupied = 1;
			table[pos].key = key;
			table[pos].val = incr;
			ht->len ++;
			ret = incr;
			break;
		} else if (table[pos].key == key) {
			table[pos].val += incr;
			ret = table[pos].val;
			break;
		}
	}

	if (ht->len > ht->load_limit)
		float_ht_rehash(ht);

	return ret;
}

static void float_ht_rehash(struct float_ht *ht)
{
	assert(ht->sz_level != TOTAL_SZ_LEVELS);
	int sz_level = ht->sz_level + 1;
	struct float_ht tmp_ht = float_ht_new(sz_level);

#ifdef DEBUG_U16_HASHTABLE
	printf("rehashing...\n");
#endif

	for (int i = 0; i < ht->sz; i++)
		if (ht->table[i].occupied)
			float_ht_update(&tmp_ht, ht->table[i].key, ht->table[i].val);

	float_ht_free(ht);
	*ht = tmp_ht;
}

void float_ht_print(struct float_ht *ht)
{
	for (int i = 0; i < ht->sz; i++)
		if (ht->table[i].occupied)
			printf("[%u] %u: %f ", i, ht->table[i].key, ht->table[i].val);

	printf(" (%d/%d/%d:%d)\n", ht->len, ht->load_limit,
	       ht->sz, ht->sz_level);
	return;
}
