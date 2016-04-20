#include <stdint.h>
#include <stddef.h> /* for size_t */

enum {
	KEYVAL_DB_OPEN_WR,
	KEYVAL_DB_OPEN_RD,
};

typedef void* keyval_db_t;

keyval_db_t keyval_db_open(const char *, int);
void        keyval_db_close(keyval_db_t);
void        keyval_db_flush(keyval_db_t);

uint64_t    keyval_db_records(keyval_db_t);
const char *keyval_db_last_err(keyval_db_t);
const char *keyval_db_version(keyval_db_t);


/* Function keyval_db_put() returns non-zero on error.
 * And if a record with the same key exists, its value
 * will be overwritten. */
int   keyval_db_put(keyval_db_t, const void *, size_t, void *, size_t);

void *keyval_db_get(keyval_db_t, const void *, size_t, size_t *);
