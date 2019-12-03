#include <assert.h>

#include "term-index/config.h" /* for excluding cache terms */
#include "dir-util/dir-util.h" /* for MAX_FILE_NAME_LEN */
#include "tex-parser/head.h" /* for optr_print() */
#include "indices.h"

static void indices_update_stats(struct indices* indices)
{
	if (indices->ti) {
		/* FIX: Uncomment below causes Indri segment fault after
		 * indexing hundreds of documents (for unknown reason). */
		// indices->n_doc = term_index_get_docN(indices->ti);

		if (!indices->avgDocLen_updated) {
			/* update once here for efficiency reason. */
			indices->avgDocLen = term_index_get_avgDocLen(indices->ti);
			indices->avgDocLen_updated = 1;
		}
	}

	if (indices->mi) {
		indices->n_tex    = indices->mi->stats.n_tex;
		indices->n_secttr = indices->mi->stats.N;
	}
}

int indices_open(struct indices* indices, const char* index_path,
                  enum indices_open_mode mode)
{
	/* return variable */
	int                   open_err = 0;

	/* path strings */
	const char            blob_index_url_name[] = "url";
	const char            blob_index_txt_name[] = "doc";
	char                  path[MAX_FILE_NAME_LEN];

	/* indices shorthand variables */
	term_index_t          term_index = NULL;
	math_index_t          math_index = NULL;
	blob_index_t          blob_index_url = NULL;
	blob_index_t          blob_index_txt = NULL;

	/* zero all fields */
	memset(indices, 0, sizeof(struct indices));

	/* set open mode */
	indices->open_mode = mode;
	indices->avgDocLen_updated = 0;

	/*
	 * open term index.
	 */
	sprintf(path, "%s/term", index_path);

	if (mode == INDICES_OPEN_RW)
		mkdir_p(path);

	term_index = term_index_open(path, (mode == INDICES_OPEN_RD) ?
	                             TERM_INDEX_OPEN_EXISTS:
	                             TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		fprintf(stderr, "cannot create/open term index.\n");
		open_err = 1;

		goto skip;
	}

	/*
	 * open math index.
	 */
	math_index = math_index_open(index_path, (mode == INDICES_OPEN_RD) ?
	                             "r": "w");
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
	                                 BLOB_OPEN_RD : BLOB_OPEN_WR);
	if (NULL == blob_index_url) {
		fprintf(stderr, "cannot create/open URL blob index.\n");

		open_err = 1;
		goto skip;
	}

	sprintf(path, "%s/%s", index_path, blob_index_txt_name);
	blob_index_txt = blob_index_open(path, (mode == INDICES_OPEN_RD) ?
	                                 BLOB_OPEN_RD : BLOB_OPEN_WR);
	if (NULL == blob_index_txt) {
		fprintf(stderr, "cannot create/open text blob index.\n");

		open_err = 1;
		goto skip;
	}

skip:
	/* assign shorthand variables */
	indices->ti = term_index;
	indices->mi = math_index;
	indices->url_bi = blob_index_url;
	indices->txt_bi = blob_index_txt;

	/* set cache memory limits */
	indices->mi_cache_limit = DEFAULT_MATH_INDEX_CACHE_SZ MB;
	indices->ti_cache_limit = DEFAULT_TERM_INDEX_CACHE_SZ MB;

	/* set index stats */
	indices_update_stats(indices);

	/* Ad-hoc FIX: Update n_doc only for initialization. */
	if (indices->ti)
		indices->n_doc = term_index_get_docN(indices->ti);

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
}

int indices_cache(struct indices* indices)
{
	if (indices->open_mode == INDICES_OPEN_RD) {
		/* only do caching in read-only mode */
		int res = 0;
		res |= math_index_load(indices->mi, indices->mi_cache_limit);
		indices->memo_usage += indices->mi->memo_usage;

		res |= term_index_load(indices->ti, indices->ti_cache_limit);
		indices->memo_usage += term_index_cache_memo_usage(indices->ti);
		return res;
	} else {
		return 1;
	}
}

void indices_print_summary(struct indices* indices)
{
	printf("[ Indices ] cached %u KB \n", indices->memo_usage);
	printf("term index: documents=%u, avg docLen=%u \n",
		indices->n_doc, indices->avgDocLen);
	printf("math index: TeXs=%u, sector trees=%u \n",
		indices->n_tex, indices->n_secttr);
}

char *get_blob_txt(blob_index_t bi, doc_id_t docID,
                   int decompress, size_t *str_len)
{
	struct codec   codec = {CODEC_GZ, NULL};
	static char    text[MAX_CORPUS_FILE_SZ + 1];
	size_t         blob_sz, text_sz;
	char          *blob_out = NULL;

	blob_sz = blob_index_read(bi, docID, (void **)&blob_out);

	if (blob_out) {
		if (decompress) {
			text_sz = codec_decompress(&codec, blob_out, blob_sz,
					text, MAX_CORPUS_FILE_SZ);
			text[text_sz] = '\0';
			*str_len = text_sz;
		} else {
			memcpy(text, blob_out, blob_sz);
			text[blob_sz] = '\0';
			*str_len = blob_sz;
		}

		blob_free(blob_out);
		return strdup(text);
	}

	fprintf(stderr, "error: get_blob_string().\n");
	*str_len = 0;
	return NULL;
}

/*
 * Below are indexer implementation.
 */
#include <ctype.h> /* for tolower() */
#include "txt-seg/lex.h" /* for g_lex_handler */

static struct indexer *g_indexer = NULL;

static void
index_blob(blob_index_t bi, doc_id_t docID, const char *str, size_t str_sz,
           bool compress)
{
	struct codec codec = {CODEC_GZ, NULL};
	size_t compressed_sz;
	void  *compressed;

	if (compress) {
		compressed_sz = codec_compress(&codec, str, str_sz, &compressed);
		blob_index_write(bi, docID, compressed, compressed_sz);
		free(compressed);
	} else {
		blob_index_write(bi, docID, str, str_sz);
	}
}

static void strip_math_tag(char *str, size_t n_bytes)
{
	size_t tag_sz = strlen("[imath]");
	uint32_t i;
	for (i = 0; tag_sz + i + 1 < n_bytes - tag_sz; i++) {
		str[i] = str[tag_sz + i];
	}

	str[i] = '\0';
}

static void eng_to_lower_case(char *str, size_t n)
{
	size_t i;
	for(i = 0; i < n; i++)
		str[i] = tolower(str[i]);
}

static struct tex_parse_ret
index_tex(math_index_t mi, char *tex, doc_id_t docID, uint32_t expID)
{
	struct tex_parse_ret ret;
#ifdef DEBUG_INDEXER
	printf("[parse tex] `%s'\n", tex);
#endif

#ifdef SUPPORT_MATH_WILDCARDS
	#error("math wildcards not supported yet.")
#else
	ret = tex_parse(tex, 0, true, true);
#endif

	if (ret.code != PARSER_RETCODE_ERR) {
		if (ret.operator_tree) {
#ifdef DEBUG_INDEXER
			optr_print((struct optr_node*)ret.operator_tree, stdout);
#endif
			optr_release((struct optr_node*)ret.operator_tree);
		}
		/* add TeX into inverted index */
		math_index_add(mi, docID, expID, ret.subpaths);
		subpaths_release(&ret.subpaths);
	}

	return ret;
}

static int indexer_handle_slice(struct lex_slice *slice)
{
	struct indices *indices = g_indexer->indices;
	size_t str_len = strlen(slice->mb_str);
	struct tex_parse_ret tex_parse_ret;

//#ifdef DEBUG_INDEXER
//	printf("input slice: [%s] <%u, %lu>\n", slice->mb_str,
//		slice->offset, str_len);
//#endif

	/* safe-guard for document length (maximum number of positions) */
	if (g_indexer->cur_position + 1 == UINT16_MAX) {
		prerr("document position reaches maximum (%u)", UINT16_MAX);
		return 1;
	}

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH_SEG:
		/* synchronize position in Indri index */
		term_index_doc_add(indices->ti, TERM_INDEX_MATH_EXPR_PLACEHOLDER);

		/* extract tex from math tag */
		strip_math_tag(slice->mb_str, str_len);

		/* index TeX */
		tex_parse_ret = index_tex(indices->mi, slice->mb_str,
		                          indices->n_doc + 1, g_indexer->cur_position);
		/* increments */
		g_indexer->n_parse_tex ++;

		if (tex_parse_ret.code == PARSER_RETCODE_ERR) {
			/* on parser error */
			g_indexer->n_parse_err++;

			parser_exception_callbk callbk = g_indexer->on_parser_exception;
			if (callbk) /* invoke parser error callback */
				callbk(g_indexer, slice->mb_str, tex_parse_ret.msg);
			return 1;
		}

		break;

	case LEX_SLICE_TYPE_ENG_SEG:
	case LEX_SLICE_TYPE_MIX_SEG:
		/* refuse to index text terms ridiculously long */
		if (str_len > INDICES_MAX_TEXT_TERM_LEN) {
			/* synchronize position in Indri index */
			term_index_doc_add(indices->ti, TERM_INDEX_LONG_WORD_PLACEHOLDER);
			break;
		}

		/* turn all indexing words to lower case for recall */
		eng_to_lower_case(slice->mb_str, str_len);

		/* add term into inverted-index */
		//printf("add [%s]@%u\n", slice->mb_str, g_indexer->cur_position);
		term_index_doc_add(indices->ti, slice->mb_str);

		break;

	default:
		assert(0);
	}

	/* current position increment */
	g_indexer->cur_position ++;
	return 0;
}

struct indexer
*indexer_alloc(struct indices *indices, text_lexer lexer,
               parser_exception_callbk on_exception)
{
	struct indexer *indexer = calloc(1, sizeof *indexer);
	indexer->indices = indices;
	indexer->lexer = lexer;
	indexer->on_parser_exception = on_exception;

	/* register global lexer handler */
	g_lex_handler = &indexer_handle_slice;
	return indexer;
}

void indexer_free(struct indexer *indexer)
{
	free(indexer);
}

uint32_t indexer_write_all_fields(struct indexer *indexer)
{
	struct indices *indices = indexer->indices;

	/* index URL field */
	index_blob(indices->url_bi, indices->n_doc + 1,
		indexer->url_field, strlen(indexer->url_field), 0);

	/* index TEXT field */
	size_t txt_sz = strlen(indexer->txt_field);
	FILE  *fh_txt = fmemopen((void *)indexer->txt_field, txt_sz, "r");

	assert(fh_txt != NULL);

	/* prepare indexing */
	term_index_doc_begin(indices->ti);

	/* invoke lexer */
	g_indexer = indexer;
	indexer->lexer(fh_txt);
	fclose(fh_txt);

	/* index TEXT blob */
	index_blob(indices->txt_bi, indices->n_doc + 1,
		indexer->txt_field, txt_sz, 1);

	/* finishing index */
	doc_id_t docID = term_index_doc_end(indices->ti);
	assert(docID == indices->n_doc + 1);

	/* update docID and indices stats */
	indices_update_stats(indices);

	/* update n_doc because it is likely to be out-of-date after
	 * indices_update_stats() due to Indri caching mechanism. */
	indices->n_doc = docID;

	/* reset lexer position */
	indexer->cur_position = 0;
	return docID;
}

int indexer_maintain(struct indexer *indexer)
{
	return term_index_should_maintain(indexer->indices->ti);
}

int indexer_should_maintain(struct indexer *indexer)
{
	term_index_maintain(indexer->indices->ti);
	return 0;
}

int indexer_spill(struct indexer *indexer)
{
	term_index_write(indexer->indices->ti);
	return 0;
}
