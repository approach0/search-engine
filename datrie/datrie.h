#pragma once
#include <stdint.h>

typedef uint32_t datrie_state_t;

struct datrie {
	datrie_state_t *base, *check;
	datrie_state_t len, max_val;
};

struct datrie datrie_new();
void datrie_free(struct datrie);

void datrie_print(struct datrie, int);

datrie_state_t datrie_cmap(const char);

datrie_state_t datrie_lookup(struct datrie*, const char*);
datrie_state_t datrie_insert(struct datrie*, const char*);
