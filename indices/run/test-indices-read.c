#include "tex-parser/vt100-color.h"
#include "indexer/config.h" // for MAX_CORPUS_FILE_SZ
#include "codec/codec.h"
#include "indices.h"
#include "config.h"

#undef N_DEBUG
#include <assert.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

struct check_pos {
	doc_id_t docID;
	position_t pos;
	struct list_node ln;
};

LIST_DEF_FREE_FUN(free_check_pos_li, struct check_pos, ln, free(p));

static LIST_IT_CALLBK(check)
{
	size_t bb_sz, dec_sz, i;

	LIST_OBJ(struct check_pos, cp, ln);
	P_CAST(indices, struct indices, pa_extra);

	offsetmap_from_t key = {cp->docID, cp->pos};
	offsetmap_to_t *val;
	size_t val_sz;

	char  *url_blob;
	char  *txt_blob;
	static char  text[MAX_CORPUS_FILE_SZ + 1]; /* too large to be in stack */
	struct codec codec = {CODEC_GZ, NULL};

	printf("Checking doc#%u, pos@%u...\n", cp->docID, cp->pos);

	/* get URL of this docID */
	bb_sz = blob_index_read(indices->url_bi, cp->docID, (void*)&url_blob);

	printf("URL: ");
	for (i = 0; i < bb_sz; i++) {
		printf("%c", url_blob[i]);
	}
	printf("\n");

	/* get offset of this term */
	bb_sz = blob_index_read(indices->txt_bi, cp->docID, (void*)&txt_blob);
	dec_sz = codec_decompress(&codec, txt_blob, bb_sz,
	                          text, MAX_CORPUS_FILE_SZ);

	val = keyval_db_get(indices->ofs_db, &key, sizeof(offsetmap_from_t),
	                    &val_sz);
	assert(val != NULL);
	printf("offset:[%u, %u]\n", val->offset, val->n_bytes);

	/* print text with highlighted term */
	text[dec_sz] = '\0';
	for (i = 0; i < dec_sz; i++) {
		if (i == val->offset)
			printf(C_CYAN);
		else if (i == val->offset + val->n_bytes)
			printf(C_RST);

		printf("%c", text[i]);
	}
	printf("\n\n");

	/* free */
	free(val);
	blob_free(url_blob);
	blob_free(txt_blob);

	LIST_GO_OVER;
}

static void add_check_pos(list *li, doc_id_t docID, position_t pos)
{
	struct check_pos *cp = malloc(sizeof(struct check_pos));
	cp->docID = docID;
	cp->pos = pos;
	LIST_NODE_CONS(cp->ln);

	list_insert_one_at_tail(&cp->ln, li, NULL, NULL);
}

int main()
{
	struct indices indices;
	const char index_path[] = "../indexer/tmp";
	list check_pos_li = LIST_NULL;

	uint32_t i, termN, termID;
	char *term;
	const uint32_t max_terms_listed = 100;
	void *posting;
	struct term_posting_item *pip;
	position_t *pos_arr;

	if(indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	termN = term_index_get_termN(indices.ti);
	printf("%u term(s) in indices, listed are top %u:\n", termN,
	       max_terms_listed);

	for (i = 1; i <= MIN(max_terms_listed, termN); i++) {
		term = term_lookup_r(indices.ti, i);
		printf("#%u: `%s' ", i, term);
	}
	printf("\n");

	printf("Please input termID:\n");
	scanf("%u", &termID);

	posting = term_index_get_posting(indices.ti, termID);
	if (posting) {
		term_posting_start(posting);

		do {
			pip = term_posting_cur_item_with_pos(posting);
			pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);

			printf("[docID=%u, tf=%u", pip->doc_id, pip->tf);

			printf(", pos=");
			for (i = 0; i < pip->tf; i++) {
				printf("%d%c", pos_arr[i], (i == pip->tf - 1) ? '.' : ',');
				add_check_pos(&check_pos_li, pip->doc_id, pos_arr[i]);
			}

			printf("]");

		} while (term_posting_next(posting));

		term_posting_finish(posting);
		printf("\n");
	}

	list_foreach(&check_pos_li, check, &indices);

	/* free check position list */
	free_check_pos_li(&check_pos_li);

close:
	indices_close(&indices);

	return 0;
}
