#include "indexer/config.h" /* for MAX_CORPUS_FILE_SZ */
#include "indexer/index.h" /* for text_lexer and indices */
#include "mem-index/mem-posting.h"

#include "config.h"
#include "postmerge.h"
#include "proximity.h"
#include "rank.h"
#include "snippet.h"
#include "search-utils.h"

#undef NDEBUG
#include <assert.h>

/*
 * get postmerge callback functions
 * (including term posting iteration function wrappers)
 */
static void *term_posting_cur_item_wrap(void *posting)
{
	return (void*)term_posting_cur_item_with_pos(posting);
}

static uint64_t term_posting_cur_item_id_wrap(void *item)
{
	doc_id_t doc_id;
	doc_id = ((struct term_posting_item*)item)->doc_id;
	return (uint64_t)doc_id;
}

static bool term_posting_jump_wrap(void *posting, uint64_t to_id)
{
	bool succ;

	/* because uint64_t value can be greater than doc_id_t,
	 * we need a wrapper function to safe-guard from
	 * calling term_posting_jump with illegal argument. */
	if (to_id >= UINT_MAX)
		succ = 0;
	else
		succ = term_posting_jump(posting, (doc_id_t)to_id);

	return succ;
}

struct postmerge_callbks *get_memory_postmerge_callbks(void)
{
	static struct postmerge_callbks ret;
	ret.start  = &mem_posting_start;
	ret.finish = &mem_posting_finish;
	ret.jump   = &mem_posting_jump;
	ret.next   = &mem_posting_next;
	ret.now    = &mem_posting_cur_item;
	ret.now_id = &mem_posting_cur_item_id;

	return &ret;
}

struct postmerge_callbks *get_disk_postmerge_callbks(void)
{
	static struct postmerge_callbks ret;
	ret.start  = &term_posting_start;
	ret.finish = &term_posting_finish;
	ret.jump   = &term_posting_jump_wrap;
	ret.next   = &term_posting_next;
	ret.now    = &term_posting_cur_item_wrap;
	ret.now_id = &term_posting_cur_item_id_wrap;

	return &ret;
}

/*
 * new rank hit
 */
struct rank_hit *new_hit(struct postmerge *pm, doc_id_t hitID,
                         float score, uint32_t n_save_occurs)
{
	uint32_t i;
	struct term_posting_item *pip /* posting item with positions */;
	struct rank_hit *hit;
	position_t *pos_arr;
	position_t *occurs;

	hit = malloc(sizeof(struct rank_hit));
	hit->docID = hitID;
	hit->score = score;
	hit->n_occurs = n_save_occurs;
	hit->occurs = occurs = malloc(sizeof(position_t) * n_save_occurs);

	for (i = 0; i < pm->n_postings; i++)
		if (pm->curIDs[i] == hitID) {
			pip = pm->cur_pos_item[i];
			pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);

			if (n_save_occurs >= pip->tf) {
				memcpy(occurs, pos_arr, pip->tf * sizeof(position_t));
				occurs += pip->tf;
				n_save_occurs -= pip->tf;
			} else {
				memcpy(occurs, pos_arr, n_save_occurs * sizeof(position_t));
				break;
			}
		}

	return hit;
}

/*
 * get blob string
 */
char *get_blob_string(blob_index_t bi, doc_id_t docID, bool gz, size_t *sz)
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
			*sz = text_sz;
		} else {
			memcpy(text, blob_out, blob_sz);
			text[blob_sz] = '\0';
			*sz = blob_sz;
		}

		blob_free(blob_out);
		return text;
	}

	fprintf(stderr, "error: get_blob_string().\n");
	*sz = 0;
	return NULL;
}

/*
 * prepare snippet
 */
typedef void (*seg_it_callbk)(char*, uint32_t, size_t, void*);

struct seg_it_args {
	uint32_t      slice_offset;
	seg_it_callbk fun;
	void         *arg;
};

struct highlighter_arg {
	position_t *pos_arr;
	uint32_t    pos_arr_now, pos_arr_sz;
	uint32_t    cur_lex_pos;
	list        hi_list; /* highlight list */
};

static void
debug_print_seg(char *mb_str, uint32_t offset, size_t sz, void *arg)
{
	printf("`%s' [%u, %lu]\n", mb_str, offset, sz);
}

static void
add_highlight_seg(char *mb_str, uint32_t offset, size_t sz, void *arg)
{
	P_CAST(ha, struct highlighter_arg, arg);

	if (ha->pos_arr_now == ha->pos_arr_sz) {
		/* all the highlight word is iterated */
		return;

	} else if (ha->cur_lex_pos == ha->pos_arr[ha->pos_arr_now]) {
		/* this is the segment of current highlight position,
		 * push it into snippet with offset information. */
		snippet_push_highlight(&ha->hi_list, mb_str, offset, sz);
		/* next highlight position */
		ha->pos_arr_now ++;
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

static void highlighter_arg_lex_setter(struct lex_slice *slice)
{
	//foreach_seg(slice, &debug_print_seg, NULL);
	foreach_seg(slice, &add_highlight_seg, &hi_arg);
}

static void bubble_sort(position_t *arr, uint32_t n)
{
	uint32_t i, j;
	position_t tmp;

	for (i = 0; i < n; i++)
		for (j = i; j < n; j++)
			if (arr[i] > arr[j]) {
				tmp = arr[i];
				arr[i] = arr[j];
				arr[j] = tmp;
			}
}

list prepare_snippet(struct rank_hit* hit, char *text, size_t text_sz, text_lexer lex)
{
	FILE    *text_fh;

	/* prepare highlighter arguments */
	hi_arg.pos_arr = hit->occurs;
	hi_arg.pos_arr_now = 0;
	hi_arg.pos_arr_sz = hit->n_occurs;
	hi_arg.cur_lex_pos = 0;
	LIST_CONS(hi_arg.hi_list);

	/* sort highlighting positions */
	bubble_sort(hit->occurs, hit->n_occurs);

	/* register lex handler  */
	g_lex_handler = highlighter_arg_lex_setter;

	/* invoke lexer */
	text_fh = fmemopen((void *)text, text_sz, "r");
	lex(text_fh);

	/* print snippet */
	snippet_read_file(text_fh, &hi_arg.hi_list);

	/* close file handler */
	fclose(text_fh);

	return hi_arg.hi_list;
}

/*
 * consider_top_K() function
 */
#define MIN(A, B) ((A) < (B) ? (A) : (B))

void
consider_top_K(ranked_results_t *rk_res,
               doc_id_t docID, float score,
               struct postmerge *pm, uint32_t n_tot_occurs)
{
	struct rank_hit *hit;
	uint32_t n_save_occurs = MIN(MAX_HIGHLIGHT_OCCURS, n_tot_occurs);

	if (!priority_Q_full(rk_res) ||
	    score > priority_Q_min_score(rk_res)) {

		hit = new_hit(pm, docID, score, n_save_occurs);
		priority_Q_add_or_replace(rk_res, hit);
	}
}
