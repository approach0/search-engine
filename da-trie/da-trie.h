#ifndef DA_TRIE_H
#define DA_TRIE_H

#include <stdio.h>
#include <wchar.h>
#include "config.h"

/*
struct da_trie {
	FILE *fh;
	char path[MAX_FILE_PATH_STR_LEN];
	// something else here
};
*/

void *da_trie_new();
void *da_trie_open(char *path);

term_id_t da_trie_map(void *, wchar_t *term, size_t len);

int da_trie_write(void *);
int da_trie_close(void *);

#endif
