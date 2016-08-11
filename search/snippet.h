void snippet_push_highlight(list*, char*, uint32_t, uint32_t);
void snippet_free_highlight_list(list*);

void snippet_read_file(FILE*, list*);

/* return a static string */
const char
*snippet_highlighted(list*, const char*, const char*);

/* print a color highlighted string in terminal */
void snippet_hi_print(list*);

/* print position info of div list (for debug) */
void snippet_pos_print(list*);
