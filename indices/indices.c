#include "indices.h"
#include "config.h"

void indices_init(struct indices* indices)
{
	indices->ti = NULL;
	indices->mi = NULL;
	indices->url_bi = NULL;
	indices->txt_bi = NULL;
	indices->postcache.trp_root = NULL;
}

bool indices_open(struct indices* indices, const char* index_path,
                  enum indices_open_mode mode)
{
	/* return variables */
	bool                  open_err = 0;

	/* temporary variables */
	const char            blob_index_url_name[] = "url";
	const char            blob_index_txt_name[] = "doc";
	char                  path[MAX_FILE_NAME_LEN];

	/* indices variables */
	void                 *term_index = NULL;
	math_index_t          math_index = NULL;
	blob_index_t          blob_index_url = NULL;
	blob_index_t          blob_index_txt = NULL;

	/* cache variables */
	struct postcache_pool postcache;
	postcache.trp_root = NULL;

	/*
	 * open term index.
	 */
	sprintf(path, "%s/term", index_path);

	mkdir_p(path);

	term_index = term_index_open(path, TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		fprintf(stderr, "cannot create/open term index.\n");
		open_err = 1;

		goto skip;
	}

	/*
	 * open math index.
	 */
	math_index = math_index_open(index_path, (mode == INDICES_OPEN_RD) ?
	                             MATH_INDEX_READ_ONLY: MATH_INDEX_WRITE);
	if (NULL == math_index) {
		fprintf(stderr, "cannot create/open math index.\n");

		open_err = 1;
		goto skip;
	}

	/*
	 * open blob index
	 */
	sprintf(path, "%s/%s", index_path, blob_index_url_name);
	blob_index_url = blob_index_open(path, (mode == INDICES_OPEN_RD) ?
	                                 "r" : "w+");
	if (NULL == blob_index_url) {
		fprintf(stderr, "cannot create/open URL blob index.\n");

		open_err = 1;
		goto skip;
	}

	sprintf(path, "%s/%s", index_path, blob_index_txt_name);
	blob_index_txt = blob_index_open(path, (mode == INDICES_OPEN_RD) ?
	                                 "r" : "w+");
	if (NULL == blob_index_txt) {
		fprintf(stderr, "cannot create/open text blob index.\n");

		open_err = 1;
		goto skip;
	}

	/* initialize posting cache pool */
	postcache_init(&postcache, 0 MB);

skip:
	indices->ti = term_index;
	indices->mi = math_index;
	indices->url_bi = blob_index_url;
	indices->txt_bi = blob_index_txt;
	indices->postcache = postcache;

	return open_err;
}

void indices_close(struct indices* indices)
{
	if (indices->ti) {
		term_index_close(indices->ti);
		indices->ti = NULL;
	}

	if (indices->mi) {
		math_index_close(indices->mi);
		indices->mi = NULL;
	}

	if (indices->url_bi) {
		blob_index_close(indices->url_bi);
		indices->url_bi = NULL;
	}

	if (indices->txt_bi) {
		blob_index_close(indices->txt_bi);
		indices->txt_bi = NULL;
	}

	if (indices->postcache.trp_root) {
		postcache_free(&indices->postcache);
	}
}


void indices_cache(struct indices* indices, uint64_t mem_limit)
{
	enum postcache_err res;
	uint32_t  termN;
	void     *posting;
	term_id_t term_id;

#ifdef ENABLE_PRINT_CACHE_TERMS
	uint32_t  df;
	char     *term;
	bool      ellp_lock = 0;
#endif

	postcache_set_mem_limit(&indices->postcache, mem_limit);

	termN = term_index_get_termN(indices->ti);

	printf("caching term postings...\n");
	for (term_id = 1; term_id <= termN; term_id++) {
		posting = term_index_get_posting(indices->ti, term_id);

		if (posting) {
#ifdef ENABLE_PRINT_CACHE_TERMS
			df = term_index_get_df(indices->ti, term_id);
			term = term_lookup_r(indices->ti, term_id);

			if (term_id < MAX_PRINT_CACHE_TERMS) {
				printf("`%s'(df=%u) ", term, df);

			} else if (!ellp_lock) {
				printf(" ...... ");
				ellp_lock = 1;
			}

			free(term);
#endif

			res = postcache_add_term_posting(&indices->postcache,
			                                 term_id, posting);
			if (res == POSTCACHE_EXCEED_MEM_LIMIT)
				break;
		}
	}
#ifdef ENABLE_PRINT_CACHE_TERMS
	printf("\n");
#endif

	printf("caching completed (%u posting lists cached):\n", term_id);
	postcache_print_mem_usage(&indices->postcache);
	printf("\n");
}
