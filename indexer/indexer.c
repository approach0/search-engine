#include <stdlib.h>
#include <unistd.h>

#include "indexer.h"
#include "config.h"

#undef NDEBUG
#include <assert.h>

static void        *term_index = NULL;
static math_index_t math_index = NULL;
static keyval_db_t  offset_db  = NULL;

static doc_id_t   prev_docID /* docID just indexed */ = 0;
static position_t cur_position = 0;

void index_set(void * ti, math_index_t mi, keyval_db_t od)
{
	term_index = ti;
	math_index = mi;
	offset_db  = od;
}

static bool save_offset(uint32_t offset, uint32_t n_bytes)
{
	offsetmap_from_t from;
	offsetmap_to_t   to;

	from.docID = prev_docID + 1;
	from.pos   = cur_position;
	to.offset  = offset;
	to.n_bytes = n_bytes;

#ifdef DEBUG_INDEXER
	printf("saving offset map: docID=%u, pos=%u => offset=%u, sz=%u ...\n",
	       prev_docID + 1, cur_position, offset, n_bytes);
#endif

	if(keyval_db_put(offset_db, &from, sizeof(offsetmap_from_t),
	                 &to, sizeof(offsetmap_to_t))) {
		printf("put error: %s\n", keyval_db_last_err(offset_db));
		return 1;
	}

	return 0;
}

static void index_tex(char *tex)
{
	struct tex_parse_ret parse_ret;
	parse_ret = tex_parse(tex, 0, false);

	if (parse_ret.code == PARSER_RETCODE_SUCC) {
#ifdef DEBUG_INDEXER
		printf("[index tex] `%s'\n", tex);
#endif

		/* actual tex indexing */
		math_index_add_tex(math_index, prev_docID + 1, cur_position,
		                   parse_ret.subpaths);
		subpaths_release(&parse_ret.subpaths);

	} else {
		printf("parsing TeX (`%s') error: %s\n", tex, parse_ret.msg);
	}

	/* increment position */
	cur_position ++;
}

static void index_term(char *term, uint32_t offset, size_t n_bytes)
{
#ifdef DEBUG_INDEXER
	/* print */
	printf("[index term] %s <%u, %u>\n", term,
	       offset, n_bytes);
#endif

	/* add term into inverted-index */
	term_index_doc_add(term_index, term);
	save_offset(offset, n_bytes);

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

void indexer_handle_slice(struct lex_slice *slice)
{
	size_t str_sz = strlen(slice->mb_str);
	list   li     = LIST_NULL;

#ifdef DEBUG_INDEXER
	printf("input slice: [%s]\n", slice->mb_str);
#endif

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH:
#ifdef DEBUG_INDEXER
		printf("[index math tag] %s <%u, %lu>\n", slice->mb_str,
		       slice->offset, str_sz);
#endif

		/* extract tex from math tag and add it into math-index */
		strip_math_tag(slice->mb_str, str_sz);
		index_tex(slice->mb_str);

		save_offset(slice->offset, str_sz);

		break;

	case LEX_SLICE_TYPE_TEXT:
		eng_to_lower_case(slice->mb_str, str_sz);

		li = text_segment(slice->mb_str);
		list_foreach(&li, &_index_term, &slice->offset);
		txt_seg_li_release(&li);

		break;

	case LEX_SLICE_TYPE_ENG_TEXT:
		eng_to_lower_case(slice->mb_str, str_sz);

		index_term(slice->mb_str, slice->offset, str_sz);
		break;

	default:
		assert(0);
	}
}

static void index_maintain()
{
	if (term_index_maintain(term_index)) {
		printf("\r[index maintaining...]");
		fflush(stdout);
		sleep(2);

		keyval_db_flush(offset_db);
	}
}

void index_file(const char *fullpath, file_lexer lex)
{
	doc_id_t docID;

	/* initialize term index */
	term_index_doc_begin(term_index);

	/* invoke lexer */
	(*lex)(fullpath);

	/* update statical variables after indexing */
	docID = term_index_doc_end(term_index);

	assert(docID == prev_docID + 1);

	prev_docID = docID;
	cur_position = 0;

	/* maintain index (e.g. optimize, merge...) */
	index_maintain();
}
