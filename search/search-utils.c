#include "common/common.h"
#include "indexer/config.h" /* for MAX_CORPUS_FILE_SZ */
#include "indexer/index.h" /* for text_lexer and indices */
#include "wstring/wstring.h" /* for wstr2mbstr() */
#include "highlight/math-expr-highlight.h"

#include "config.h"
#include "proximity.h"
#include "rank.h"
#include "snippet.h"
#include "search-utils.h"

#undef NDEBUG
#include <assert.h>

/*
 * get blob string
 */
char
*get_blob_string(blob_index_t bi, doc_id_t docID, bool gz, size_t *str_len)
{
	struct codec   codec = {CODEC_GZ, NULL};
	static char    text[MAX_CORPUS_FILE_SZ + 1];
	size_t         blob_sz, text_sz;
	char          *blob_out = NULL;

	blob_sz = blob_index_read(bi, docID, (void **)&blob_out);

	if (blob_out) {
		if (gz) {
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
 * prepare snippet
 */
#include "txt-seg/lex.h"
#define LEXER_FUN lex_eng_file

typedef void (*seg_it_callbk)(char*, uint32_t, size_t, void*);

struct seg_it_args {
	uint32_t      slice_offset;
	seg_it_callbk fun;
	void         *arg;
};

struct highlighter_arg {
	hit_occur_t *positions;
	uint32_t     positions_cur, positions_len;
	uint32_t     cur_lex_pos;
	list         hi_list; /* highlight list */
};

static void
debug_print_seg_offset(char *mb_str, uint32_t offset, size_t sz, void *arg)
{
	printf("`%s' [%u, %lu]\n", mb_str, offset, sz);
}

static void
debug_print_highlight_seg(char *mb_str, uint32_t offset,
                          size_t sz, void *arg)
{
	P_CAST(ha, struct highlighter_arg, arg);
	uint32_t i, n = ha->positions_len;

	printf("%u: `%s' ", ha->cur_lex_pos, mb_str);
	for (i = 0; i < n; i++) {
		printf("%u-", ha->positions[i].pos);
	}

	printf("\n");
	ha->cur_lex_pos ++;
}

static void
add_highlight_seg(char *mb_str, uint32_t offset, size_t sz, void *arg)
{
	P_CAST(ha, struct highlighter_arg, arg);

	if (ha->positions_cur == ha->positions_len) {
		/* all the highlight word is iterated */
		return;

	} else if (ha->cur_lex_pos == ha->positions[ha->positions_cur].pos) {
		/* this is the segment of current highlight position,
		 * push it into snippet with offset information. */
#ifdef HIGHLIGHT_MATH_ALIGNMENT
		if (ha->positions[ha->positions_cur].qmask[0]) {
			/* this is math keyword */
			uint64_t *qmask = ha->positions[ha->positions_cur].qmask;
			uint64_t *dmask = ha->positions[ha->positions_cur].dmask;
			strip_math_tag(mb_str, sz); // strip [imath] tags
			char *hi_tex = math_oprand_highlight(mb_str, dmask, MAX_MTREE);
			char hi_kw[strlen(hi_tex) + 512 /* additional chars below */];
#ifdef HIGHLIGHT_MATH_W_DEGREE
			snprintf(hi_kw, sizeof(hi_kw),
				"<span class=\"score\">%lu</span>"
				"<span class=\"dmask\">%lx,%lx,%lx</span>"
				"<span class=\"expid\">%u</span>[imath]%s[/imath]",
				qmask[0], dmask[0], dmask[1], dmask[2],
				ha->cur_lex_pos, hi_tex
			);
#else
			snprintf(hi_kw, sizeof(hi_kw),
				"<span class=\"qmask\">%lx,%lx,%lx</span>"
				"<span class=\"dmask\">%lx,%lx,%lx</span>"
				"<span class=\"expid\">%u</span>[imath]%s[/imath]",
				qmask[0], qmask[1], qmask[2],
				dmask[0], dmask[1], dmask[2],
				ha->cur_lex_pos, hi_tex
			);
#endif
			snippet_push_highlight(&ha->hi_list, hi_kw, offset, sz);
		} else {
			/* this is text keyword */
			snippet_push_highlight(&ha->hi_list, mb_str, offset, sz);
		}
#else
		/* in this case, they are all handled in the same way. */
		snippet_push_highlight(&ha->hi_list, mb_str, offset, sz);
#endif

		/* next highlight position */
		ha->positions_cur ++;
	}

	/* next position */
	ha->cur_lex_pos ++;
}

static LIST_IT_CALLBK(seg_iteration)
{
	LIST_OBJ(struct text_seg, seg, ln);
	P_CAST(sia, struct seg_it_args, pa_extra);

	/* adjust offset relatively to file */
	seg->offset += sia->slice_offset;

	/* call the callback function */
	sia->fun(seg->str, seg->offset, seg->n_bytes, sia->arg);

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(free_txt_seg_list, struct text_seg,
                  ln, free(p));

static void
foreach_seg(struct lex_slice *slice, seg_it_callbk fun, void *arg)
{
	size_t str_sz = strlen(slice->mb_str);
	list   li     = LIST_NULL;
	struct seg_it_args sia = {slice->offset, fun, arg};

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH_SEG:
		/* this is a math segment */

		fun(slice->mb_str, slice->offset, str_sz, arg);
		break;

	case LEX_SLICE_TYPE_MIX_SEG:
		/* this is a mixed segment (e.g. English and Chinese) */

		/* need to segment further */
		li = text_segment(slice->mb_str);
		list_foreach(&li, &seg_iteration, &sia);
		free_txt_seg_list(&li);

		break;

	case LEX_SLICE_TYPE_ENG_SEG:
		/* this is a pure English segment */

		fun(slice->mb_str, slice->offset, str_sz, arg);
		break;

	default:
		assert(0);
	}
}

/* highlighter arguments */
static struct highlighter_arg hi_arg;

static int highlighter_arg_lex_setter(struct lex_slice *slice)
{
#ifdef DEBUG_HILIGHT_SEG_OFFSET
	foreach_seg(slice, &debug_print_seg_offset, NULL);
#endif

#ifdef DEBUG_HILIGHT_SEG
	foreach_seg(slice, &debug_print_highlight_seg, &hi_arg);
#else
	foreach_seg(slice, &add_highlight_seg, &hi_arg); /* add highlight */
#endif

	return 0;
}

list
prepare_snippet(struct rank_hit* hit, const char *text, size_t text_sz)
{
	FILE *text_fh;

	/* prepare highlighter arguments */
	hi_arg.positions = hit->occurs;
	hi_arg.positions_cur = 0;
	hi_arg.positions_len = hit->n_occurs;
	hi_arg.cur_lex_pos = 0;
	LIST_CONS(hi_arg.hi_list);

	/* register lex handler  */
	g_lex_handler = highlighter_arg_lex_setter;

	/* invoke lexer */
	text_fh = fmemopen((void *)text, text_sz, "r");
	LEXER_FUN(text_fh);

	/* print snippet */
	snippet_read_file(text_fh, &hi_arg.hi_list);

	/* close file handler */
	fclose(text_fh);

	return hi_arg.hi_list;
}

/* Below are debug functions */
void
print_math_expr_at(struct indices *indices, doc_id_t docID, exp_id_t expID)
{
	char  *str;
	size_t str_sz;
	list   highlight_list;
#ifdef HIGHLIGHT_MATH_ALIGNMENT
	hit_occur_t pos[1] = {{expID,{0},{0}}};
#else
	hit_occur_t pos[1] = {{expID}};
#endif
	struct rank_hit mock_hit = {docID, 0, 0, 0, 1, pos};

	printf("expr#%u @ doc#%u:\n", expID, docID);

	g_lex_handler = highlighter_arg_lex_setter;

	str = get_blob_string(indices->txt_bi, docID, 1, &str_sz);
	highlight_list = prepare_snippet(&mock_hit, str, str_sz);
	free(str);

	snippet_hi_print(&highlight_list);
	snippet_free_highlight_list(&highlight_list);
}

static void print_res_item(struct rank_hit* hit, int cnt, void *arg)
{
	char  *str;
	size_t str_sz;
	list   highlight_list;
	PTR_CAST(indices, struct indices, arg);
	printf("page result#%u: doc#%u score=%.3f\n", cnt, hit->docID, hit->score);

	/* get URL */
	str = get_blob_string(indices->url_bi, hit->docID, 0, &str_sz);
	printf("URL: %s" "\n", str);
	free(str);

	printf("\n");

	/* get document text */
	str = get_blob_string(indices->txt_bi, hit->docID, 1, &str_sz);

	/* prepare highlighter arguments */
	highlight_list = prepare_snippet(hit, str, str_sz);
	free(str);

	/* print snippet */
	snippet_hi_print(&highlight_list);
	printf("--------\n\n");

	/* free highlight list */
	snippet_free_highlight_list(&highlight_list);
}

void
print_search_results(ranked_results_t *rk_res, int page,
                     struct indices *indices)
{
	struct rank_window wind;
	int i, from_page = page, to_page = page, tot_pages = 1;
	wind = rank_window_calc(rk_res, 0, DEFAULT_RES_PER_PAGE, &tot_pages);

	if (page == 0) {
		from_page = 1;
		to_page = tot_pages;
	}

	for (i = from_page - 1; i < MIN(to_page, tot_pages); i++) {
		printf("page %u/%u\n", i + 1, tot_pages);
		wind = rank_window_calc(rk_res, i, DEFAULT_RES_PER_PAGE, &tot_pages);
		rank_window_foreach(&wind, &print_res_item, indices);
	}
}

/* math tex extraction function */
static char _expr_lex_str[MAX_CORPUS_FILE_SZ + 1];
static uint32_t _expr_lex_cur = 0;
static uint32_t _expr_lex_pos = 0;

static int _expr_lex_getter(struct lex_slice *slice)
{
	if (_expr_lex_pos == _expr_lex_cur) {
		strcpy(_expr_lex_str, slice->mb_str);
		strip_math_tag(_expr_lex_str, strlen(_expr_lex_str));
		_expr_lex_pos = 0; /* touch flag */
	}
	_expr_lex_cur ++;
	return 0;
}

char *get_expr_by_pos(char *text, size_t text_sz, uint32_t pos)
{
	_expr_lex_cur = 0;
	_expr_lex_pos = pos;
	g_lex_handler = _expr_lex_getter;
	FILE *text_fh = fmemopen((void *)text, text_sz, "r");
	LEXER_FUN(text_fh);
	fclose(text_fh);

	if (_expr_lex_pos == 0)
		return _expr_lex_str;
	else
		return NULL;
}
