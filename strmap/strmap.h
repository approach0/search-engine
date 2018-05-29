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

struct strmap *strmap_new();
void strmap_free(struct strmap*);
void **strmap_val_ptr(struct strmap*, char*);
