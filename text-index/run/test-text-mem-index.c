#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mhook/mhook.h"

#include "list/list.h"
#include "tree/treap.h"
#include "datrie/datrie.h"

#include "wstring/wstring.h"
#include "txt-seg/lex.h"

#include "mem-index/mem-posting.h"

#define TEXT_INDEX_MAX_DOC_TERMS (65536)

struct text_index_post_item {
	uint32_t docID;
	uint32_t tf;
};

typedef struct text_index_term {
	struct mem_posting *posting;
	struct treap_node   trp_nd;
	uint32_t            df, cur_doc_tf;
} text_index_term_t;

struct text_index_segment {
	struct treap_node *trp_root;
	struct datrie     *dict;
	uint32_t           cur_docID, cur_word_pos, cur_term_cnt;
	datrie_state_t     cur_doc_terms[TEXT_INDEX_MAX_DOC_TERMS];
};

static struct text_index_segment s_idx_seg;

void text_index_segment_push_word(struct text_index_segment*, char *);

static int my_lex_handler(struct lex_slice *slice)
{
	uint32_t n_bytes = strlen(slice->mb_str);

	switch (slice->type) {
	case LEX_SLICE_TYPE_ENG_SEG:
		eng_to_lower_case(slice->mb_str, n_bytes);
		// printf("%s <%u, %u>\n", slice->mb_str, slice->offset, n_bytes);

		text_index_segment_push_word(&s_idx_seg, slice->mb_str);
		break;

	default:
		break;
	}

	return 0;
}

struct text_index_segment
text_index_segment_new(struct datrie *dict)
{
	struct text_index_segment ret = {0};
	ret.cur_docID = 1; /* docID starting from 1 */
	ret.dict = dict; /* save the global dict */

	rand_timeseed(); /* new random seed */
	return ret;
}

static enum bintr_it_ret
text_index_free_posting(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct text_index_term *index_term =
		MEMBER_2_STRUCT(ref->this_, struct text_index_term, trp_nd.bintr_nd);
	P_CAST(po, struct mem_posting, index_term->posting);

	bintr_detach(ref->this_, ref->ptr_to_this);
	if (po) mem_posting_free(po);
	free(index_term);

	return BINTR_IT_CONTINUE;
}

void
text_index_segment_free(struct text_index_segment *seg)
{
	bintr_foreach((struct bintr_node **)&seg->trp_root,
	              &bintr_postorder, &text_index_free_posting, NULL);
}

void
text_index_segment_push_word(struct text_index_segment* seg, char *word)
{
	struct treap_node         *mapped_node;
	struct text_index_term    *mapped_term;

	/* dictionary/node map */
	datrie_state_t termID;
	termID = datrie_lookup(seg->dict, word);
	//datrie_print(*seg->dict, 0);

	if (termID == 0) {
		termID = datrie_insert(seg->dict, word);
#ifdef TEXT_MEM_INDEX_DEBUG
		printf("inserted a new term `%s' (assigned termID#%u)\n",
		       word, termID);
#endif
		/* insert new treap node */
		mapped_term = calloc(1, sizeof(struct text_index_term));
		TREAP_NODE_CONS(mapped_term->trp_nd, termID);
		mapped_node = treap_insert(&seg->trp_root, &mapped_term->trp_nd);
	} else {
#ifdef TEXT_MEM_INDEX_DEBUG
		printf("mapped to existing termID#%u\n", termID);
#endif
		mapped_node = treap_find(seg->trp_root, termID);

		/* casting node to term structure */
		assert(NULL != mapped_node);
		mapped_term = MEMBER_2_STRUCT(mapped_node, text_index_term_t, trp_nd);
	}

	/* update current word position */
	seg->cur_word_pos ++;

	/* if a new unique term found for this document */
	if (0 == mapped_term->cur_doc_tf) {
		/* record all unique terms in this document */
		seg->cur_doc_terms[seg->cur_term_cnt ++] = termID;
	}

	/* accumulate term frequency */
	mapped_term->cur_doc_tf ++;
}

void text_index_segment_doc_end(struct text_index_segment* seg)
{
	uint32_t i, termID;
	struct treap_node         *mapped_node;
	struct text_index_term    *mapped_term;
	struct text_index_post_item post_item;

#ifdef TEXT_MEM_INDEX_DEBUG
	printf("doc#%u end: \n", seg->cur_docID);
#endif

	/* for each unique document term */
	for (i = 0; i < seg->cur_term_cnt; i++) {
		termID = seg->cur_doc_terms[i];
		mapped_node = treap_find(seg->trp_root, termID);

		/* casting node to term structure */
		assert(NULL != mapped_node);
		mapped_term = MEMBER_2_STRUCT(mapped_node, text_index_term_t, trp_nd);

		/* append to the corresponding posting list */
		post_item.docID = seg->cur_docID;
		post_item.tf = mapped_term->cur_doc_tf;

		if (NULL == mapped_term->posting) {
			mapped_term->posting = mem_posting_create(DEFAULT_SKIPPY_SPANS,
			                       mem_term_posting_codec_calls());
		}
		mem_posting_write(mapped_term->posting, &post_item, sizeof(post_item));

		/* update document frequency for this term */
		mapped_term->df ++;
		/* reset previous document tf counter */
		mapped_term->cur_doc_tf = 0;

#ifdef TEXT_MEM_INDEX_DEBUG
		printf("term#%u (df=%u) appends: [doc#%u, tf=%u]\n",
		       termID, mapped_term->df, post_item.docID, post_item.tf);
#endif
	}

	/* reset numbers for the next document */
	seg->cur_docID ++;
	seg->cur_word_pos = 0;
	seg->cur_term_cnt = 0;
}

static enum bintr_it_ret
print_term_info(struct bintr_ref *ref, uint32_t level, void *arg)
{
	text_index_term_t *node;
	bintr_key_t key = ref->this_->key;
	node = MEMBER_2_STRUCT(ref->this_, text_index_term_t,
	                       trp_nd.bintr_nd);
	printf("term#%u df: %u \n", key, node->df);
	return BINTR_IT_CONTINUE;
}

void text_index_print_terms_info(struct text_index_segment* seg)
{
	if (seg->trp_root == NULL)
		return;

	struct bintr_node *bintr_nd = &seg->trp_root->bintr_nd;
	bintr_foreach((struct bintr_node **)&bintr_nd, &bintr_inorder,
	              &print_term_info, NULL);
}

static enum bintr_it_ret
posting_flush(struct bintr_ref *ref, uint32_t level, void *arg)
{
	text_index_term_t *node;
	node = MEMBER_2_STRUCT(ref->this_, text_index_term_t,
	                       trp_nd.bintr_nd);
	mem_posting_write_complete(node->posting);
	return BINTR_IT_CONTINUE;
}

void text_index_segment_end(struct text_index_segment* seg)
{
	if (seg->trp_root == NULL)
		return;

	struct bintr_node *bintr_nd = &seg->trp_root->bintr_nd;
	bintr_foreach((struct bintr_node **)&bintr_nd, &bintr_inorder,
	              &posting_flush, NULL);
}

void testcase_index_txtfile(const char *fname)
{
	FILE *fh = fopen(fname, "r");
	if (fh == NULL) {
		printf("cannot open `%s'...\n", fname);
		return;
	}

	lex_eng_file(fh);
	text_index_segment_doc_end(&s_idx_seg);
	fclose(fh);
}

void print_posting(struct text_index_segment *seg,
                   struct datrie *dict, const char *keyword)
{
	struct treap_node       *node;
	struct text_index_term  *term;
	datrie_state_t           termID;
	struct mem_posting      *po;

	termID = datrie_lookup(dict, keyword);
	if (termID == 0) {
		printf("[undefined]");
		return;
	}
	node = treap_find(seg->trp_root, termID);
	term = MEMBER_2_STRUCT(node, text_index_term_t, trp_nd);
	po = term->posting;

	if (0 == mem_posting_start(po)) {
		printf("[empty]");
		goto skip;
	}

	do {
		struct text_index_post_item *pi;
		pi = mem_posting_cur_item(po);
		printf("[docID=%u, tf=%u] ", pi->docID, pi->tf);
	} while (mem_posting_next(po));

skip:
	mem_posting_finish(po);
}

int main()
{
	int cnt = 0;
	struct datrie dict = datrie_new();
	g_lex_handler = my_lex_handler;
	s_idx_seg = text_index_segment_new(&dict);

	//while (cnt < 9) {
		testcase_index_txtfile("test/1.txt");
		testcase_index_txtfile("test/2.txt");
		cnt ++;
	//}
	text_index_segment_end(&s_idx_seg);

	printf("enter keywords ...\n");
	{
		int i, n_keywords = 0;
		char query[128][128];
		while (EOF != scanf("%s", query[n_keywords])) {
			n_keywords ++;
		}

		for (i = 0; i < n_keywords; i++) {
			printf("keyword[%d]: %s \n", i, query[i]);
			print_posting(&s_idx_seg, &dict, query[i]);
			printf("\n");
		}
	}

	text_index_segment_free(&s_idx_seg);

	// datrie_print(dict, 0);
	datrie_free(dict);

	mhook_print_unfree();
	return 0;
}
