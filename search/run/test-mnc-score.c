#include "mhook/mhook.h"
#include "tex-parser/tex-parser.h" /* for symbol_id_t */
#include "tex-parser/gen-symbol.h"
#include "mnc-score.h"

static symbol_id_t alphabet_to_sym(char c)
{
	int inc = (int)(c - 'a');
	return S_N + inc;
}

int main(void)
{
	int i;
	struct mnc_ref ref = {'_', '@'};
	mnc_score_t score;
	uint32_t slot;

	mnc_reset_qry();

	/*
	 * query: b + b + 1/b = a + a
	 *        1   0   5 2   4   3
	 */
	ref.sym = alphabet_to_sym('b'); /* 0 */
	mnc_push_qry(ref);
	ref.sym = alphabet_to_sym('b'); /* 1 */
	mnc_push_qry(ref);
	ref.sym = alphabet_to_sym('b'); /* 2 */
	mnc_push_qry(ref);
	ref.sym = alphabet_to_sym('a'); /* 3 */
	mnc_push_qry(ref);
	ref.sym = alphabet_to_sym('a'); /* 4 */
	mnc_push_qry(ref);
	ref.sym = S_one; /* 5 */
	mnc_push_qry(ref);

	/* run twice to test init/uninit */
	for (i = 0; i < 2; i++) {
		printf("======test %d=======\n", i);
		mnc_reset_docs();

		/*
		 * document: y + y + x = 1/x + x<--(use macro below to remove this x)
		 *           0   1   2   3 4   5
		 */
		ref.sym = alphabet_to_sym('x'); /* 2 */
		slot = mnc_map_slot(ref);
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(slot, 2, 1);
		mnc_doc_add_rele(slot, 2, 0);
		mnc_doc_add_rele(slot, 2, 4);
		mnc_doc_add_rele(slot, 2, 3);

		ref.sym = alphabet_to_sym('y'); /* 1 */
		slot = mnc_map_slot(ref);
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(slot, 1, 1);
		mnc_doc_add_rele(slot, 1, 0);
		mnc_doc_add_rele(slot, 1, 4);
		mnc_doc_add_rele(slot, 1, 3);

		ref.sym = S_one; /* 3 */
		slot = mnc_map_slot(ref);
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(slot, 3, 5);

		ref.sym = alphabet_to_sym('x'); /* 4 */
		slot = mnc_map_slot(ref);
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(slot, 4, 2);

		ref.sym = alphabet_to_sym('y'); /* 0 */
		slot = mnc_map_slot(ref);
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(slot, 0, 3);
		mnc_doc_add_rele(slot, 0, 1);
		mnc_doc_add_rele(slot, 0, 4);
		mnc_doc_add_rele(slot, 0, 0);

//#define TEST_EARLY_TERMINATION
#ifndef TEST_EARLY_TERMINATION
		ref.sym = alphabet_to_sym('x'); /* 5 */
		slot = mnc_map_slot(ref);
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(slot, 5, 4);
		mnc_doc_add_rele(slot, 5, 1);
		mnc_doc_add_rele(slot, 5, 3);
		mnc_doc_add_rele(slot, 5, 0);
#endif

		score = mnc_score();
		printf("score = %u.\n", score);
	}

	printf("lsb position of %#x: %d\n", 0x18, lsb_pos(0x18)); /* b11000 */
	printf("lsb position of %#x: %d\n", 0x01, lsb_pos(0x01));
	printf("lsb position of %#x: %d\n", 0x00, lsb_pos(0x00));

	mhook_print_unfree();
	return 0;
}
