#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "common/common.h"
#include "term-index.hpp"
#include "term-index.h"

using namespace std;

enum {
	TI_DOCID,
	TI_TF,
	TI_N_OCCUR,
	TI_POS_ARR
};

struct codec_buf_struct_info *term_codec_info()
{
	struct codec_buf_struct_info *info;
	info = codec_buf_struct_info_alloc(4, sizeof(struct term_posting_item));

#define SET_FIELD_INFO(_i, _name, _codec) \
	info->field_info[_i] = FIELD_INFO(struct term_posting_item, _name, _codec)

	SET_FIELD_INFO(TI_DOCID, doc_id, CODEC_FOR); // CODEC_FOR_DELTA);
	SET_FIELD_INFO(TI_TF, tf, CODEC_FOR);
	SET_FIELD_INFO(TI_N_OCCUR, n_occur, CODEC_FOR);
	SET_FIELD_INFO(TI_POS_ARR, n_occur, CODEC_FOR);

	return info;
}

static struct invlist
*fork_term_postlist(void *disk_invlist, codec_buf_struct_info_t *cinfo)
{
	struct invlist *memo_invlist;

	memo_invlist = invlist_open(NULL, TERM_INDEX_BLK_LEN, cinfo);
	invlist_iter_t  memo_writer = invlist_writer(memo_invlist);

	/* reset on-disk term posting list to the begining */
	assert (0 != term_posting_start(disk_invlist));

	do {
		struct term_posting_item item;
		/* get docID, TF and position array */
		term_posting_read(disk_invlist, &item);

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

	index->cinfo = term_codec_info();
	index->trp_root = NULL;
	index->memo_usage = 0;

	for (term_id = 1; term_id <= termN; term_id++) {
		void *disk_invlist = term_index_get_posting(index_, term_id);
		if (NULL == disk_invlist)
			continue;

		struct invlist *memo_invlist;
		memo_invlist = fork_term_postlist(disk_invlist, index->cinfo);

		/* get document frequency */
		uint32_t df = term_index_get_df(index, term_id);

#ifdef PRINT_CACHING_TEXT_TERMS
		char *term = term_lookup_r(index, term_id);
		printf("caching `%s' (df=%u, size=%lu KB)\n", term, df,
			memo_invlist->tot_sz / 1024);
		free(term);
#endif

		/* insert this in-memory inverted list into cache */
		{
			struct treap_node *inserted;
			struct term_index_cache_entry *entry;
			entry = (struct term_index_cache_entry*)malloc(sizeof *entry);
			entry->invlist = memo_invlist;
			entry->df = df;
			TREAP_NODE_CONS(entry->trp_nd, term_id);

			inserted = treap_insert(&index->trp_root, &entry->trp_nd);
			assert(inserted != NULL);
		}

		/* update term index memory usage */
		index->memo_usage += sizeof(struct treap_node);
		index->memo_usage += memo_invlist->tot_sz;

		if (index->memo_usage > limit_sz)
			break;
	}

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
	codec_buf_struct_info_free(index->cinfo);

	bintr_foreach((struct bintr_node **)&index->trp_root,
	              &bintr_postorder, &free_cache_entry, index_);

	index->memo_usage = 0;
}

struct term_invlist_entry_reader
term_index_lookup(void *index_, term_id_t term_id)
{
	struct term_invlist_entry_reader entry_reader = {0};
	P_CAST(index, struct term_index, index_);

	if (index_ == NULL || term_id == 0)
		return entry_reader;

	struct bintr_ref ref;
	ref = bintr_find((struct bintr_node**)&index->trp_root, term_id);

	if (ref.this_) {
		/* found in the cached index */
		struct term_index_cache_entry *entry;
		entry = container_of(ref.this_, struct term_index_cache_entry,
			trp_nd.bintr_nd);

		entry_reader.inmemo_reader = invlist_iterator(entry->invlist);
		entry_reader.ondisk_reader = NULL;
		entry_reader.df = entry->df;
	} else {
		void *ondisk_reader = term_index_get_posting(index_, term_id);
		if (ondisk_reader) {
			entry_reader.inmemo_reader = NULL;
			entry_reader.ondisk_reader = ondisk_reader;
			entry_reader.df = term_index_get_df(index_, term_id);
		}
	}

	return entry_reader;
}
