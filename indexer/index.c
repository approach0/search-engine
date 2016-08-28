#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "parson/parson.h"
#include "tex-parser/vt100-color.h"
#include "index.h"
#include "config.h"

#undef NDEBUG
#include <assert.h>

static void        *term_index = NULL;
static math_index_t math_index = NULL;
static blob_index_t blob_index_url = NULL;
static blob_index_t blob_index_txt = NULL;

static doc_id_t   prev_docID /* docID just indexed */ = 0;
static position_t cur_position = 0;

uint64_t n_parse_err = 0;
uint64_t n_parse_tex = 0;

doc_id_t indexer_assign(struct indices *indices)
{
	uint32_t max_docID;

	term_index = indices->ti;
	math_index = indices->mi;
	blob_index_url = indices->url_bi;
	blob_index_txt = indices->txt_bi;

	max_docID = term_index_get_docN(term_index);
	prev_docID = max_docID;

	return max_docID;
}

static void
index_blob(blob_index_t bi, const char *str, size_t str_sz, bool compress)
{
	struct codec codec = {CODEC_GZ, NULL};
	size_t compressed_sz;
	void  *compressed;

#ifdef DEBUG_INDEXER
	printf("indexing blob:\n""%s\n", str);
#endif

	if (compress) {
		compressed_sz = codec_compress(&codec, str, str_sz, &compressed);

#ifdef DEBUG_INDEXER
		printf("compressed from %lu into %lu bytes.\n", str_sz, compressed_sz);
#endif
		blob_index_write(bi, prev_docID + 1, compressed, compressed_sz);
		free(compressed);
	} else {
#ifdef DEBUG_INDEXER
		printf("not compressed.\n");
#endif

		blob_index_write(bi, prev_docID + 1, str, str_sz);
	}
}

static int
index_tex(char *tex, uint32_t offset, size_t n_bytes)
{
	int ret;
	struct tex_parse_ret parse_ret;
#ifdef DEBUG_INDEXER
	printf("[parse tex] `%s'\n", tex);
#endif
	parse_ret = tex_parse(tex, 0, false);

	if (parse_ret.code != PARSER_RETCODE_ERR) {
		/* warning or completely successful */
#ifdef DEBUG_INDEXER
		printf("[index tex] `%s'\n", tex);
#endif
		/* actual tex indexing */
		math_index_add_tex(math_index, prev_docID + 1,
		                   cur_position, parse_ret.subpaths);
		subpaths_release(&parse_ret.subpaths);

		ret = 0; /* successful */
	} else {
		/* grammar error or too many subpaths */
		fprintf(stderr, C_RED "`%s': %s\n" C_RST,
		        tex, parse_ret.msg);

		ret = 1;
	}

	/* increment position */
	cur_position ++;

	return ret;
}

static void index_term(char *term, uint32_t offset, size_t n_bytes)
{
#ifdef DEBUG_INDEXER
	/* print */
	printf("[index term] %s <%u, %lu>\n", term,
	       offset, n_bytes);
#endif

	/* add term into inverted-index */
	term_index_doc_add(term_index, term);

	/* increment position */
	cur_position ++;
}

static LIST_IT_CALLBK(_index_term)
{
	LIST_OBJ(struct text_seg, seg, ln);

	/* adjust offset relatively to file */
	P_CAST(slice_offset, uint32_t, pa_extra);
	seg->offset += *slice_offset;

	index_term(seg->str, seg->offset, seg->n_bytes);

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(txt_seg_li_release, struct text_seg,
                  ln, free(p));

int indexer_handle_slice(struct lex_slice *slice)
{
	size_t str_sz = strlen(slice->mb_str);
	list   li     = LIST_NULL;

#ifdef DEBUG_INDEXER
	printf("input slice: [%s]\n", slice->mb_str);
#endif

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH_SEG:
#ifdef DEBUG_INDEXER
		printf("[index math tag] %s <%u, %lu>\n", slice->mb_str,
		       slice->offset, str_sz);
#endif
		/* term_index_doc_add() is invoked here to make position numbers
		 * synchronous in both math-index and Indri. */
		term_index_doc_add(term_index, "math_exp");

		/* extract tex from math tag and add it into math-index */
		strip_math_tag(slice->mb_str, str_sz);

		/* count how many TeX parsed */
		n_parse_tex ++;

		if (index_tex(slice->mb_str, slice->offset, str_sz)) {
			n_parse_err++;
			return 1;
		}

		break;

	case LEX_SLICE_TYPE_MIX_SEG:
		eng_to_lower_case(slice->mb_str, str_sz);

		li = text_segment(slice->mb_str);
		list_foreach(&li, &_index_term, &slice->offset);
		txt_seg_li_release(&li);

		break;

	case LEX_SLICE_TYPE_ENG_SEG:
		eng_to_lower_case(slice->mb_str, str_sz);

		index_term(slice->mb_str, slice->offset, str_sz);
		break;

	default:
		assert(0);
	}

	return 0;
}

static void index_maintain()
{
	if (term_index_maintain(term_index)) {
		printf("\r[index maintaining...]");
		fflush(stdout);
		sleep(2);
	}
}

static bool get_json_val(const char *json, const char *key, char *val)
{
	JSON_Value *parson_val = json_parse_string(json);
	JSON_Object *parson_obj;

	if (parson_val == NULL) {
		json_value_free(parson_val);
		return 0;
	}

	parson_obj = json_value_get_object(parson_val);
	strcpy(val, json_object_get_string(parson_obj, key));

	json_value_free(parson_val);
	return 1;
}

static int index_text_field(const char *txt, text_lexer lex)
{
	doc_id_t docID;
	size_t txt_sz = strlen(txt);
	FILE *fh_txt = fmemopen((void *)txt, txt_sz, "r");
	int ret;

	/* safe check */
	if (fh_txt == NULL) {
		perror("fmemopen() function.");
		exit(EXIT_FAILURE);
	}

	/* prepare indexing a document */
	term_index_doc_begin(term_index);

	/* invoke lexer */
	ret = (*lex)(fh_txt);

	/* index text blob */
	index_blob(blob_index_txt, txt, txt_sz, 1);

	/* close memory file handler */
	fclose(fh_txt);

	/* done indexing this document */
	docID = term_index_doc_end(term_index);
	assert(docID == prev_docID + 1);

	/* update document indexing variables */
	prev_docID = docID;
	cur_position = 0;

	return ret;
}

int indexer_index_json(FILE *fh, text_lexer lex)
{
	static char doc_json[MAX_CORPUS_FILE_SZ + 1];
	static char url_field[MAX_CORPUS_FILE_SZ];
	static char txt_field[MAX_CORPUS_FILE_SZ];
	size_t      rd_sz;
	int         ret;

	rd_sz = fread(doc_json, 1, MAX_CORPUS_FILE_SZ, fh);
	doc_json[rd_sz] = '\0';

	if (rd_sz == MAX_CORPUS_FILE_SZ) {
		fprintf(stderr, "corpus file too large!\n");
		return 1;
	}

	if (!get_json_val(doc_json, "url", url_field)) {
		fprintf(stderr, "JSON: get URL field failed.\n");
		return 1;
	}

	if (!get_json_val(doc_json, "text", txt_field)) {
		fprintf(stderr, "JSON: get URL field failed.\n");
		return 1;
	}

	/* URL blob indexing, it is done prior than text indexing
	 * because prev_docID is not updated at this point. */
	index_blob(blob_index_url, url_field, strlen(url_field), 0);

	/* text indexing */
	ret = index_text_field(txt_field, lex);

	/* maintain index (e.g. optimize, merge...) */
	index_maintain();

	return ret;
}

static int foreach_file_callbk(const char *filename, void *arg)
{
	P_CAST(cnt, uint64_t, arg);

	if (json_ext(filename)) {
		(*cnt) ++;
	}

	return 0;
}

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	P_CAST(cnt, uint64_t, arg);
	foreach_files_in(path, &foreach_file_callbk, cnt);

	return DS_RET_CONTINUE;
}

uint64_t total_json_files(const char *dir)
{
	uint64_t cnt = 0;
	if (dir_exists(dir))
		dir_search_podfs(dir, &dir_search_callbk, &cnt);

	return cnt;
}
