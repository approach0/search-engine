#ifndef TERM_LOOKUP_H
#define TERM_LOOKUP_H

#include <stdio.h>
#include <wchar.h>
#include "../config/config.h"

void *term_lookup_open(char *path);

term_id_t term_lookup(void *, wchar_t *term);

int term_lookup_flush(void *);
int term_lookup_close(void *);

#endif
