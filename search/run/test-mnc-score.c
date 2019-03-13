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
	struct mnc_match_t match;

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
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(1, 2, ref);
		mnc_doc_add_rele(0, 2, ref);
		mnc_doc_add_rele(4, 2, ref);
		mnc_doc_add_rele(3, 2, ref);

		ref.sym = alphabet_to_sym('y'); /* 1 */
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(1, 1, ref);
		mnc_doc_add_rele(0, 1, ref);
		mnc_doc_add_rele(4, 1, ref);
		mnc_doc_add_rele(3, 1, ref);

		ref.sym = S_one; /* 3 */
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(5, 3, ref);

		ref.sym = alphabet_to_sym('x'); /* 4 */
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(2, 4, ref);

		ref.sym = alphabet_to_sym('y'); /* 0 */
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(3, 0, ref);
		mnc_doc_add_rele(1, 0, ref);
		mnc_doc_add_rele(4, 0, ref);
		mnc_doc_add_rele(0, 0, ref);

//#define TEST_EARLY_TERMINATION
#ifndef TEST_EARLY_TERMINATION
		ref.sym = alphabet_to_sym('x'); /* 5 */
		/* write corresponding relevance_bitmap column */
		mnc_doc_add_rele(4, 5, ref);
		mnc_doc_add_rele(1, 5, ref);
		mnc_doc_add_rele(3, 5, ref);
		mnc_doc_add_rele(0, 5, ref);
#endif

		match = mnc_match();
		printf("score = %u, qry_paths: 0x%lx, doc_paths: 0x%lx.\n",
			match.score, match.qry_paths, match.doc_paths);
	}

	printf("lsb position of %#x: %d\n", 0x18, lsb_pos(0x18)); /* b11000 */
	printf("lsb position of %#x: %d\n", 0x01, lsb_pos(0x01));
	printf("lsb position of %#x: %d\n", 0x00, lsb_pos(0x00));

	mhook_print_unfree();
	return 0;
}
