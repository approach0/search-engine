/* This is the bitmap-style implementation of mnc score (or "mark
 * and cross" score) algorithm. This algorithm evaluates two math
 * expressions' similarity based on their subpaths, and also taking
 * symbolic alpha-equivalence into consideration.
 *
 * See the following papers for more details:
 * https://github.com/tkhost/tkhost.github.io/tree/master/opmes
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common/common.h"
#include "tex-parser/tex-parser.h" /* for symbol_id_t */

/* for trans_symbol() */
#include "tex-parser/vt100-color.h"
#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"
#include "tex-parser/gen-symbol.h"
#include "tex-parser/trans.h"

#include "mnc-score.h"

/*
 * statical local variables
 */

/* query expression ordered subpaths/symbols list */
symbol_id_t     qry_sym[MAX_SUBPATH_ID];
mnc_finpr_t     qry_fnp[MAX_SUBPATH_ID];
int             qry_wil[MAX_SUBPATH_ID];
uint32_t        n_qry_syms;

/* document expression unique symbols list */
symbol_id_t     doc_uniq_sym[MAX_DOC_UNIQ_SYM];
mnc_finpr_t     doc_fnp[MAX_SUBPATH_ID];
mnc_finpr_t     doc_wild_fnp[MAX_DOC_UNIQ_SYM];
uint32_t        n_doc_uniq_syms;

/* query / document bitmaps */
mnc_slot_t      doc_mark_bitmap[MAX_DOC_UNIQ_SYM];
mnc_slot_t      doc_cross_bitmap;
mnc_slot_t      relevance_bitmap[MAX_SUBPATH_ID][MAX_DOC_UNIQ_SYM];

/*
 * implementation functions
 */

/* push query (should be pushed in the order of query path symbol) */
uint32_t
mnc_push_qry(struct mnc_ref qry_path_ref, int is_wildcard)
{
	qry_sym[n_qry_syms] = qry_path_ref.sym;
	qry_fnp[n_qry_syms] = qry_path_ref.fnp;
	qry_wil[n_qry_syms] = is_wildcard;
	return (++ n_qry_syms);
}

/* return slot index that this document path belongs */
static uint32_t map_slot(symbol_id_t doc_sym)
{
	uint32_t i;
	/* find the symbol slot to which doc_path belongs. */
	for (i = 0; i < n_doc_uniq_syms; i++)
		if (doc_uniq_sym[i] == doc_sym)
			break;

	/* not found, append a new slot and add the doc_path */
	if (i == n_doc_uniq_syms) {
		doc_uniq_sym[i] = doc_sym;
		n_doc_uniq_syms ++;
	}

	return i;
}

/* set the corresponding bit to indicate relevance between two paths */
void mnc_doc_add_rele(uint32_t qry_path, uint32_t doc_path,
                      struct mnc_ref doc_path_ref)
{
	/* setup doc symbolic relevance map */
	uint32_t slot = map_slot(doc_path_ref.sym);
	mnc_slot_t bit = 1;
	bit = bit << doc_path;
	relevance_bitmap[qry_path][slot] |= bit;

	/* save doc path fingerprint */
	doc_fnp[doc_path] = doc_path_ref.fnp;
}

/* set the corresponding bits to indicate relevance between
 * a wildcard query path and a document gener path. */
void mnc_doc_add_reles(uint32_t qry_path, mnc_slot_t doc_paths,
                      struct mnc_ref doc_path_ref)
{
	/* setup doc symbolic relevance map */
	uint32_t slot = map_slot(doc_path_ref.sym);
	relevance_bitmap[qry_path][slot] |= doc_paths;

	/* save gener path fingerprint by slot */
	doc_wild_fnp[slot] = doc_path_ref.fnp;
}

/*
 * print functions for debug
 */
#define _MAX_SYMBOL_STR_LEN 16

#define _MARGIN_PAD printf(" ")

static void print_slot(unsigned char *byte)
{
	for (int i = 7; i >= 0; i--) {
		printf("%02x", byte[i]);
	}
	_MARGIN_PAD;
}

void mnc_print(mnc_score_t *sub_score,
               int highlight_qry_path, int max_subscore_idx)
{
	uint32_t i, j;
	if (n_doc_uniq_syms == 0)
		goto print_bitmap;

	/* print scores */
	printf("Max sub-score: %u from doc symbol `%s'\n",
	       sub_score[max_subscore_idx],
	       trans_symbol_wo_font(doc_uniq_sym[max_subscore_idx]));

	printf("%-*s", _MAX_SYMBOL_STR_LEN, "Score: ");
	for (i = 0; i < n_doc_uniq_syms; i++) {
		printf("%-*u ", _MAX_SYMBOL_STR_LEN, sub_score[i]);
		_MARGIN_PAD;
	}
	printf("\n");

	/* print document symbol slots */
	printf("%*c", _MAX_SYMBOL_STR_LEN, ' ');
	for (i = 0; i < n_doc_uniq_syms; i++) {
		printf("%-*s ", _MAX_SYMBOL_STR_LEN,
			trans_symbol_wo_font(doc_uniq_sym[i]));
		_MARGIN_PAD;
	}
	printf("\n");

	/* print mark and cross rows */
	printf("%-*s", _MAX_SYMBOL_STR_LEN, "Cross:");
	print_slot((char*)&doc_cross_bitmap);
	printf("\n");

	printf("%-*s", _MAX_SYMBOL_STR_LEN, "Mark: ");
	for (i = 0; i < n_doc_uniq_syms; i++)
		print_slot((char*)&doc_mark_bitmap[i]);
	printf("\n");

print_bitmap:

	/* print main bitmaps */
	for (i = 0; i < n_qry_syms; i++) {
		if ((uint32_t)highlight_qry_path == i)
			printf("-> %-*s", _MAX_SYMBOL_STR_LEN - 3,
			       trans_symbol_wo_font(qry_sym[i]));
		else
			printf("%-*s", _MAX_SYMBOL_STR_LEN,
			       trans_symbol_wo_font(qry_sym[i]));

		for (j = 0; j < n_doc_uniq_syms; j++)
			print_slot((char*)&relevance_bitmap[i][j]);

		printf("\n");
	}
}

/*
 * cleaning functions.
 */

/* reset query */
void mnc_reset_qry()
{
	n_qry_syms = 0;
}

/* reset document */
void mnc_reset_docs()
{
	n_doc_uniq_syms = 0;
}

/* clean bitmaps to the n_qry_syms, n_doc_uniq_syms dimension */
static void clean_bitmaps(void)
{
	uint32_t i;

	/* No need to clean 'mark bitmap' because cross() function
	 * already ensures a clean 'mark bitmap' after main function. */
	// memset(doc_mark_bitmap, 0, sizeof(mnc_slot_t) * n_doc_uniq_syms);

	doc_cross_bitmap = 0x0;

	for (i = 0; i < n_qry_syms; i++) {
		memset(relevance_bitmap[i], 0,
		       sizeof(mnc_slot_t) * n_doc_uniq_syms);
	}
}

/* return the index of least significant bit position */
int lsb_pos(uint64_t v)
{
	/* gcc built-in function which returns one plus the index
	 * of the least significant 1-bit of v, or if v is zero,
	 * returns zero. This is an O(1) algorithm using De-Bruijn
	 * sequence. */
	return __builtin_ctzl(v);
}

/*
 * mark and cross algorithm main functions.
 */
static __inline mnc_score_t mark(int i, int j)
{
	mnc_slot_t mark  = doc_mark_bitmap[j];
	mnc_slot_t cross = doc_cross_bitmap;
	mnc_slot_t rels = relevance_bitmap[i][j];

	/* get relevance bitmap without conflict bits */
	mnc_slot_t unmark = rels & ~(mark | cross);

	/* if there is no relevant bits left ... */
	if (unmark == 0)
		return 0;

	int fnp_equal;

	if (qry_wil[i]) {
		/* wildcard path */

		if (unmark == rels) {
			/* no conflict at all */

			/* mark all relevance bits */
			doc_mark_bitmap[j] |= rels;
		} else {
			/* won't match on any conflict */
			return 0;
		}

		/* wildcard match fingerprint score */
		if (qry_fnp[i] == doc_wild_fnp[j])
			fnp_equal = 0x1;
		else
			fnp_equal = 0x0;

	} else {
		/* concrete path */

		/* extract the lowest set bit (only need to mark one),
		 * set this mark bit on doc_mark_bitmap */
		doc_mark_bitmap[j] |= unmark & ~(unmark - 1);

		/* fingerpirnt score */
		uint32_t lo_doc_path = lsb_pos(unmark);
		if (qry_fnp[i] == doc_fnp[lo_doc_path])
			fnp_equal = 0x1;
		else
			fnp_equal = 0x0;
	}

	/* symbol score */
	int sym_equal = (qry_sym[i] == doc_uniq_sym[j]) ? 0x2 : 0x0;

	/* return score */
	switch (sym_equal | fnp_equal) {
	case 0x3:
		return MNC_MARK_FULL_SCORE; /* exact match */
		break;
	case 0x2:
		return MNC_MARK_MID_SCORE;  /* match except for fingerprint */
		break;
	case 0x1:
		return MNC_MARK_MID_SCORE;  /* match fingerprint but not symbol */
		break;
	default:
		return MNC_MARK_BASE_SCORE;
	}
}

static __inline mnc_slot_t cross(int max_slot)
{
	/* rule out the document path in the "max" slot */
	doc_cross_bitmap |= doc_mark_bitmap[max_slot];

	/* clear 'mark' bitmap */
	memset(doc_mark_bitmap, 0, sizeof(mnc_slot_t) * n_doc_uniq_syms);

	return doc_cross_bitmap;
}

struct mnc_match_t mnc_match()
{
	struct mnc_match_t ret;
	uint32_t i, j, max_subscore_idx = 0;
	mnc_slot_t qry_cross_bitmap = 0;

	mnc_score_t mark_score, total_score = 0;
	mnc_score_t max_subscore = 0;

	/* query / document slot sub-scores */
	mnc_score_t doc_uniq_sym_score[MAX_DOC_UNIQ_SYM] = {0};

#ifdef MNC_DEBUG
	/* print initial state */
	mnc_print(doc_uniq_sym_score, -1, max_subscore_idx);
#endif

	for (i = 0; i < n_qry_syms; i++) {
		for (j = 0; j < n_doc_uniq_syms; j++) {
			mark_score = mark(i, j);

			if (mark_score != 0) {
				qry_cross_bitmap |= (1L << i); /* mask this query path */
				doc_uniq_sym_score[j] += mark_score;
				if (doc_uniq_sym_score[j] > max_subscore) {
					max_subscore = doc_uniq_sym_score[j];
					max_subscore_idx = j;
				}
			}
		}

		if (n_qry_syms == i + 1 || /* this is the final iteration */
		    qry_sym[i + 1] != qry_sym[i] /* next symbol is different */) {

#ifdef MNC_DEBUG
			/* print before cross */
			mnc_print(doc_uniq_sym_score, i, max_subscore_idx);
#endif

			if (cross(max_subscore_idx) == (0L - 1))
				break; /* early termination */

			/* accumulate into total score */
			total_score += max_subscore;

#ifdef MNC_DEBUG
			/* print after cross */
			printf(C_RED "Current total score: %u\n" C_RST, total_score);
			printf("=========\n");
#endif

			/* clean sub-scores */
			memset(doc_uniq_sym_score, 0,
			       sizeof(mnc_score_t) * n_doc_uniq_syms);
			max_subscore = 0;
			max_subscore_idx = 0;
		}
#ifdef MNC_DEBUG
		else {
			/* print */
			mnc_print(doc_uniq_sym_score, i, max_subscore_idx);
			printf("~~~~~~~~~\n");
		}
#endif
	}

	/* save return values before clean_bitmaps() */
	ret = ((struct mnc_match_t) {
		total_score,
		qry_cross_bitmap, doc_cross_bitmap
	});

	clean_bitmaps();

	return ret;
}

struct mnc_match_t mnc_match_debug()
{
	struct mnc_match_t ret;
	uint32_t i, j, max_subscore_idx = 0;
	mnc_slot_t qry_cross_bitmap = 0;

	mnc_score_t mark_score, total_score = 0;
	mnc_score_t max_subscore = 0;

	/* query / document slot sub-scores */
	mnc_score_t doc_uniq_sym_score[MAX_DOC_UNIQ_SYM] = {0};

	/* print initial state */
	mnc_print(doc_uniq_sym_score, -1, max_subscore_idx);

	for (i = 0; i < n_qry_syms; i++) {
		for (j = 0; j < n_doc_uniq_syms; j++) {
			mark_score = mark(i, j);

			if (mark_score != 0) {
				qry_cross_bitmap |= (1L << i); /* mask this query path */
				doc_uniq_sym_score[j] += mark_score;
				if (doc_uniq_sym_score[j] > max_subscore) {
					max_subscore = doc_uniq_sym_score[j];
					max_subscore_idx = j;
				}
			}
		}

		if (n_qry_syms == i + 1 || /* this is the final iteration */
		    qry_sym[i + 1] != qry_sym[i] /* next symbol is different */) {

			/* print before cross */
			mnc_print(doc_uniq_sym_score, i, max_subscore_idx);

			if (cross(max_subscore_idx) == (0L - 1))
				break; /* early termination */

			/* accumulate into total score */
			total_score += max_subscore;

			/* print after cross */
			printf(C_RED "Current total score: %u\n" C_RST, total_score);
			printf("=========\n");

			/* clean sub-scores */
			memset(doc_uniq_sym_score, 0,
			       sizeof(mnc_score_t) * n_doc_uniq_syms);
			max_subscore = 0;
			max_subscore_idx = 0;
		}
		else {
			/* print */
			mnc_print(doc_uniq_sym_score, i, max_subscore_idx);
			printf("~~~~~~~~~\n");
		}
	}

	/* save return values before clean_bitmaps() */
	ret = ((struct mnc_match_t) {
		total_score,
		qry_cross_bitmap, doc_cross_bitmap
	});

	clean_bitmaps();

	return ret;
}
