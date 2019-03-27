#pragma once

#include <stdint.h>
#include "datrie/datrie.h"

struct strmap_entry {
	char *keystr;
	void *value;
};

struct strmap {
	uint32_t             length;
	uint32_t             n_entries;
	struct strmap_entry *entries;
	struct datrie        dat;
};

typedef struct strmap *strmap_t;

struct strmap *strmap_new();
void           strmap_free(struct strmap*);
void         **strmap_val_ptr(struct strmap*, char*);
void          *strmap_lookup(strmap_t, char*);
struct strmap *strmap_va_list(int, ...);

#include "ppnarg.h"
#define strmap(...) \
	strmap_va_list(PP_NARG(__VA_ARGS__), __VA_ARGS__)

typedef struct strmap_iterator {
	int idx;
	struct strmap_entry *cur;
	struct strmap *strmap;
} *strmap_iter_t;

strmap_iter_t strmap_iterator(strmap_t);
int  strmap_empty(strmap_t);
void strmap_iter_free(strmap_iter_t);
int  strmap_iter_next(strmap_iter_t);
