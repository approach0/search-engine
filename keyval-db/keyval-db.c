#include <tcutil.h>
#include <tcbdb.h>
#include "keyval-db.h"
#include "config.h"

keyval_db_t keyval_db_open(const char *path, int mode)
{
	TCBDB *bdb = tcbdbnew();

	if (mode == KEYVAL_DB_OPEN_WR) {
		if (!tcbdbopen(bdb, path, BDBOCREAT | BDBOWRITER))
			return NULL;
	} else if (mode == KEYVAL_DB_OPEN_RD) {
		if (!tcbdbopen(bdb, path, BDBOREADER))
			return NULL;
	}

	return bdb;
}

void keyval_db_close(keyval_db_t db)
{
	tcbdbclose((TCBDB*)db);
	tcbdbdel((TCBDB*)db);
}

void keyval_db_flush(keyval_db_t db)
{
	tcbdbsync((TCBDB*)db);
}

uint64_t keyval_db_records(keyval_db_t db)
{
	return tcbdbrnum((TCBDB*)db);
}

const char *keyval_db_last_err(keyval_db_t db)
{
	return tcbdberrmsg(tcbdbecode(db));
}

const char *keyval_db_version(keyval_db_t db)
{
	return tcversion;
}

int keyval_db_put(keyval_db_t db,
                  const void *key, size_t key_sz,
                  void *val, size_t val_sz)
{
	if (tcbdbput((TCBDB *)db, key, key_sz, val, val_sz))
		return 0; /* no error */
	else
		return 1;
}

void *keyval_db_get(keyval_db_t db, const void *key, size_t key_sz,
                    size_t *val_sz)
{
	int   _val_sz;
	void *ret;
	ret = tcbdbget((TCBDB *)db, key, key_sz, &_val_sz);
	*val_sz = (size_t)_val_sz;
	return ret;
}

#include <string.h>
keyval_db_t
keyval_db_open_under(const char *name, const char *path, int mode)
{
	keyval_db_t ret;
	char *loc = (char *)malloc(MAX_DIR_PATH_NAME_LEN);

	strcpy(loc, path);
	strcat(loc, "/");
	strcat(loc, name);

	ret = keyval_db_open(loc, mode);
	free(loc);

	return ret;
}
