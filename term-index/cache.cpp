#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "common/common.h"
#include "term-index.hpp"
#include "term-index.h"

struct codec_buf_struct_info *term_codec_info()
{
	struct codec_buf_struct_info *info;
	info = codec_buf_struct_info_alloc(4, sizeof(struct term_posting_item));

#define SET_FIELD_INFO(_i, _name, _codec) \
	info->field_info[_i] = FIELD_INFO(struct term_posting_item, _name, _codec)

	SET_FIELD_INFO(TI_DOCID, doc_id, CODEC_FOR); // CODEC_FOR_DELTA);
	SET_FIELD_INFO(TI_TF, tf, CODEC_FOR);
	SET_FIELD_INFO(TI_N_OCCUR, n_occur, CODEC_FOR);
	SET_FIELD_INFO(TI_POS_ARR, pos_arr, CODEC_FOR);

	return info;
}

static struct invlist
*fork_term_postlist(void *disk_invlist, codec_buf_struct_info_t *cinfo)
{
	struct invlist *memo_invlist;

	memo_invlist = invlist_open(NULL, TERM_INDEX_BLK_LEN, cinfo);
	invlist_iter_t  memo_writer = invlist_writer(memo_invlist);

	/* reset on-disk term posting list to the begining */
	int succ = term_posting_start(disk_invlist);
	(void)succ;
	assert(succ);

	do {
		struct term_posting_item item;
		/* for better compression */
		memset(item.pos_arr, 0, sizeof item.pos_arr);
		/* get docID, TF and position array */
		term_posting_read(disk_invlist, &item);

#ifdef PRINT_TERM_INDEX_CACHE_WRITES
		printf("tf=%u, n_occur = %u\n", item.tf, item.n_occur);
		for (int i = 0; i < MAX_TERM_ITEM_POSITIONS; i++) {
			printf("%u ", item.pos_arr[i]);
		}
		printf("\n");
#endif
		/* copy to memory version */
		invlist_writer_write(memo_writer, &item);

	} while (term_posting_next(disk_invlist));

	/* finishing ... */
	term_posting_finish(disk_invlist);
	invlist_writer_flush(memo_writer);

	invlist_iter_free(memo_writer);
	return memo_invlist;
}

int term_index_load(void *index_, size_t limit_sz)
{
	P_CAST(index, struct term_index, index_);

	term_id_t term_id, termN;
	termN = term_index_get_termN(index_);

#ifdef PRINT_CACHING_TEXT_TERMS
	printf("Term-index caching DocFreq threshold = %u\n",
		MINIMAL_CACHE_DOC_FREQ);
#endif

	if (NULL == index->cinfo)
		index->cinfo = term_codec_info();

	/* get IDs of excluding cache terms */
	term_id_t math_expr_id;
	term_id_t long_word_id;
	math_expr_id = term_lookup(index, TERM_INDEX_MATH_EXPR_PLACEHOLDER);
	long_word_id = term_lookup(index, TERM_INDEX_LONG_WORD_PLACEHOLDER);

	for (term_id = 1; term_id <= termN; term_id++) {
		/* break when we do not want any term being cached */
		if (limit_sz == 0) break;

		/* ignore those excluding cache terms */
		if (term_id == math_expr_id || term_id == long_word_id)
			continue;

		/* only document terms with DF greater than a threshold */
		uint32_t df = term_index_get_df(index, term_id);
		if (df < MINIMAL_CACHE_DOC_FREQ)
			continue;

		/* get on-disk inverted list for this term ID */
		void *disk_invlist = term_index_get_posting(index_, term_id);
		if (NULL == disk_invlist)
			continue;

		struct invlist *memo_invlist;
		memo_invlist = fork_term_postlist(disk_invlist, index->cinfo);

#ifdef PRINT_CACHING_TEXT_TERMS
		char *term = term_lookup_r(index, term_id);
		printf(ES_RESET_LINE);
		printf("caching `%s' (df=%u, blocks=%lu) ", term, df,
			memo_invlist->n_blk);
		fflush(stdout);
		free(term);
#endif

		/* insert this in-memory inverted list into cache */
		{
			struct treap_node *inserted;
			struct term_index_cache_entry *entry;
			entry = (struct term_index_cache_entry*)malloc(sizeof *entry);
			entry->invlist = memo_invlist;
			TREAP_NODE_CONS(entry->trp_nd, term_id);

			inserted = treap_insert(&index->trp_root, &entry->trp_nd);
			assert(inserted != NULL);
			(void)inserted;
		}

		/* update term index memory usage */
		index->memo_usage += sizeof(struct treap_node);
		index->memo_usage += sizeof *memo_invlist;
		index->memo_usage += memo_invlist->tot_payload_sz;

		if (index->memo_usage > limit_sz)
			break;
	}

#ifdef PRINT_CACHING_TEXT_TERMS
	printf("\n");
#endif
	return 0;
}

static enum bintr_it_ret
free_cache_entry(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct term_index_cache_entry *entry = container_of(
		ref->this_, struct term_index_cache_entry, trp_nd.bintr_nd
	);

	bintr_detach(ref->this_, ref->ptr_to_this);
	invlist_free(entry->invlist);
	free(entry);

	return BINTR_IT_CONTINUE;
}

void term_index_cache_free(void *index_)
{
	P_CAST(index, struct term_index, index_);
	/* free codec info */
	if (index->cinfo)
		codec_buf_struct_info_free(index->cinfo);

	/* free treap entries */
	if (index->trp_root) {
		bintr_foreach((struct bintr_node **)&index->trp_root,
			&bintr_postorder, &free_cache_entry, index_);
	}

	/* reset memory usage */
	index->memo_usage = 0;
}

struct term_invlist_entry_reader
term_index_lookup(void *index_, term_id_t term_id)
{
	struct term_invlist_entry_reader entry_reader = {0};
	P_CAST(index, struct term_index, index_);

	if (index_ == NULL || term_id == 0)
		return entry_reader;

	/* find from cached index first */
	struct bintr_ref ref;
	ref = bintr_find((struct bintr_node**)&index->trp_root, term_id);

	if (ref.this_) {
		/* found in the cached index */
		struct term_index_cache_entry *entry;
		entry = container_of(ref.this_, struct term_index_cache_entry,
			trp_nd.bintr_nd);

		/* return entry reader as iterator */
		entry_reader.inmemo_reader = invlist_iterator(entry->invlist);
		entry_reader.ondisk_reader = NULL;
	} else {
		void *ondisk_reader = term_index_get_posting(index_, term_id);
		term_posting_start(ondisk_reader);

		/* return entry reader as term_posting */
		entry_reader.inmemo_reader = NULL;
		entry_reader.ondisk_reader = ondisk_reader;
	}

	return entry_reader;
}

uint32_t term_index_cache_memo_usage(void* index_)
{
	P_CAST(index, struct term_index, index_);
	return index->memo_usage;
}
