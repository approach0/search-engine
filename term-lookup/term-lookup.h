#ifndef DA_TRIE_H
#define DA_TRIE_H

#include <stdio.h>
#include <wchar.h>
#include "config.h"

void *term_lookup_open(char *path);

term_id_t term_lookup(void *, wchar_t *term);

int term_lookup_flush(void *);
int term_lookup_close(void *);

#endif
