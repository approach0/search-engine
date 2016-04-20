#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "keyval-db.h"

int main()
{
	uint8_t  key;
	uint32_t val, *pval;
	size_t   val_sz;
	keyval_db_t db = keyval_db_open("./test.bin", KEYVAL_DB_OPEN_RD);

	if (db == NULL) {
		printf("open error.\n");
		return 1;
	}

	printf("libtokyocabinet version: %s\n", keyval_db_version(db));
	printf("total records: %lu\n", keyval_db_records(db));

	key = 0;
	val = 128;
	if (keyval_db_put(db, &key, sizeof(uint8_t), &val, sizeof(uint32_t))) {
		printf("put error: %s\n", keyval_db_last_err(db));
		printf("(fails by purpose, because we open DB using flag "
		       "KEYVAL_DB_OPEN_RD)\n");
	}

	for (key = 0; key < 8; key++) {
		if(NULL == (pval = keyval_db_get(db, &key, sizeof(uint8_t), &val_sz))) {
			printf("get error: %s\n", keyval_db_last_err(db));
			continue;
		}

		printf("%u => %u (value size = %lu)\n", key, *pval, val_sz);
		free(pval);
	}

	keyval_db_close(db);
	return 0;
}
