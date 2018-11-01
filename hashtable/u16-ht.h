#include <stdint.h>

struct u16_ht_entry {
	int occupied:  1;
	uint16_t key: 15;
	uint16_t val;
};

struct u16_ht {
	struct u16_ht_entry *table;
	int len, load_limit, sz;
	int sz_level;
};

struct u16_ht u16_ht_new(int);

void u16_ht_free(struct u16_ht*);

int u16_ht_lookup(struct u16_ht*, int);

void u16_ht_insert(struct u16_ht*, int, int);

void u16_ht_reset(struct u16_ht*, int);

void u16_ht_print(struct u16_ht*);
