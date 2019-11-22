struct example_invlist {
	int    len;
	int   *dat;
	float *score;
	float  upper; /* upperbound */
};

typedef struct {
	int                     idx;
	struct example_invlist *inv;
} *example_invlist_iter_t;

int                    example_invlist_empty(struct example_invlist*);
example_invlist_iter_t example_invlist_iterator(struct example_invlist*);

void         example_invlist_iter_free(example_invlist_iter_t);
uint64_t     example_invlist_iter_cur (example_invlist_iter_t);
int          example_invlist_iter_next(example_invlist_iter_t);
int          example_invlist_iter_skip(example_invlist_iter_t, uint64_t);
size_t       example_invlist_iter_read(example_invlist_iter_t, void*, size_t);
