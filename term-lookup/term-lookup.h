#ifndef TERM_LOOKUP_H
#define TERM_LOOKUP_H

#include <stdio.h>
#include <wchar.h>
#include "../config/config.h"

/*
open dbname and return the connection
*/
void *term_lookup_open(char *dbname);

/*
if term exists in the database, return term id
otherwise, insert the term into the database.
*/
term_id_t term_lookup(void *, wchar_t *term);

int term_lookup_flush(void *);
int term_lookup_close(void *);

#endif
