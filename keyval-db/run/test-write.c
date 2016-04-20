#include <stdio.h>
#include <stdint.h>
#include "keyval-db.h"

int main()
{
	uint8_t  key;
	uint32_t val;
	keyval_db_t db = keyval_db_open("./test.bin", KEYVAL_DB_OPEN_WR);

	if (db == NULL) {
		printf("open error.\n");
		return 1;
	}

	key = 0;
	val = 128;
	if (keyval_db_put(db, &key, sizeof(uint8_t), &val, sizeof(uint32_t))) {
		printf("put error: %s\n", keyval_db_last_err(db));
	}

	for (key = 0; key < 8; key++) {
		val = (uint32_t)(key * key);
		keyval_db_flush(db);
		if(keyval_db_put(db, &key, sizeof(uint8_t), &val, sizeof(uint32_t)))
			printf("put error: %s\n", keyval_db_last_err(db));
	}

	keyval_db_close(db);
	printf("done.\n");
	return 0;
}
