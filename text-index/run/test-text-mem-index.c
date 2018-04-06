#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"

#include "list/list.h"
#include "tree/treap.h"
#include "datrie/datrie.h"

#include "wstring/wstring.h"
#include "txt-seg/lex.h"

#include "mem-index/mem-posting.h"

#define MAX_TEX_INDEX_TERM_LEN 1024

struct text_index_term {
	uint32_t docID;
	uint32_t tf;
	char     str[MAX_TEX_INDEX_TERM_LEN];
};

struct text_index_posting {
	void                    *posting;
	struct treap_node        trp_nd;
	uint32_t                 df;
};

struct text_index_segment {
	struct treap_node *trp_root;
	struct datrie     *dict;
};

static struct text_index_segment s_idx_seg;

static int my_lex_handler(struct lex_slice *slice)
{
	uint32_t n_bytes = strlen(slice->mb_str);

	switch (slice->type) {
	case LEX_SLICE_TYPE_ENG_SEG:
		eng_to_lower_case(slice->mb_str, n_bytes);
		printf("%s <%u, %u>\n", slice->mb_str, slice->offset, n_bytes);
		break;

	default:
		break;
	}

	return 0;
}

struct text_index_segment
text_index_segment_new(struct datrie *dict)
{
	struct text_index_segment ret = {NULL, dict};
	return ret;
}

static enum bintr_it_ret
text_index_free_posting(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct text_index_posting *txt_idx_po =
		MEMBER_2_STRUCT(ref->this_, struct text_index_posting, trp_nd.bintr_nd);
	P_CAST(po, struct mem_posting, txt_idx_po->posting);

	bintr_detach(ref->this_, ref->ptr_to_this);
	mem_posting_free(po);
	free(txt_idx_po);

	return BINTR_IT_CONTINUE;
}

void
text_index_segment_del(struct text_index_segment *seg)
{
	bintr_foreach((struct bintr_node **)&seg->trp_root,
	              &bintr_postorder, &text_index_free_posting, NULL);
}

void
text_index_segment_add(struct text_index_segment* seg,
                       struct text_index_term *term)
{
	/* dictionary map */
	datrie_state_t termID;
	termID = datrie_lookup(seg->dict, term->str);
	if (termID == 0) {
		termID = datrie_insert(seg->dict, term->str);
		printf("inserted a new term `%s' (assigned termID#%u)\n",
		       term->str, termID);
	}

	/* posting list map */
//	uint32_t docID;
//	uint32_t tf;
}

int main()
{
	struct datrie dict;
	const char test_file_name[] = "test/1.txt";
	FILE *fh = fopen(test_file_name, "r");

	if (fh == NULL) {
		printf("cannot open `%s'...\n", test_file_name);
		return 1;
	}

	dict = datrie_new();
	s_idx_seg = text_index_segment_new(&dict);
	
	g_lex_handler = my_lex_handler;
	lex_eng_file(fh);

	text_index_segment_del(&s_idx_seg);
	datrie_free(dict);

	fclose(fh);
	mhook_print_unfree();
	return 0;
}
