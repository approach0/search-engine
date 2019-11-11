#pragma once
#include <stdint.h>

struct float_ht_entry {
	uint16_t occupied;
	uint16_t key;
	float    val;
};

struct float_ht {
	struct float_ht_entry *table;
	int len, load_limit, sz;
	int sz_level;
};

struct float_ht float_ht_new(int);

void float_ht_free(struct float_ht*);

float float_ht_lookup(struct float_ht*, int);

void float_ht_update(struct float_ht*, int, float);

float float_ht_incr(struct float_ht*, int, float);

void float_ht_reset(struct float_ht*, int);

void float_ht_print(struct float_ht*);
