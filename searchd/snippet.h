struct snippet_pos {
	uint32_t doc_pos /* in bytes */, n_bytes;
	char    *keystr; /* for print/debug */
	struct list_node ln;
};

struct snippet {
	char  *str; /* in multi-bytes, static allocated */
	uint32_t begin, offset /* to file head */, len; /* in bytes */
};

void snippet_rst_pos(list*);
void snippet_add_pos(list*, char*, uint32_t, uint32_t);

struct snippet snippet_read(FILE*, list*);
struct snippet snippet_read_blob(void*, size_t, list*);

/* return an allocated string */
char *snippet_highlight(struct snippet*, list*, char*, char*);;
