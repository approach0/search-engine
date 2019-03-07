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

#include "config.h"
#include "mnc-score.h"

/* define max number of slots (bound variables) */
#define MAX_DOC_UNIQ_SYM MAX_SUBPATH_ID

#ifdef MNC_SMALL_BITMAP
/* for debug */
typedef uint8_t  mnc_slot_t;
#define MNC_SLOTS_BYTES 1
#else
/* for real */
typedef uint64_t mnc_slot_t;
#define MNC_SLOTS_BYTES 8
#endif

/* finger print */
typedef uint16_t mnc_finpr_t;

/*
 * statical local variables
 */

/* query expression ordered subpaths/symbols list */
symbol_id_t     qry_sym[MAX_SUBPATH_ID];
mnc_finpr_t     qry_fnp[MAX_SUBPATH_ID];
uint32_t        n_qry_syms;

/* document expression unique symbols list */
symbol_id_t     doc_uniq_sym[MAX_DOC_UNIQ_SYM];
mnc_finpr_t     doc_fnp[MAX_SUBPATH_ID];
uint32_t        n_doc_uniq_syms;

/* query / document bitmaps */
mnc_slot_t      doc_mark_bitmap[MAX_DOC_UNIQ_SYM];
mnc_slot_t      doc_cross_bitmap[MAX_DOC_UNIQ_SYM];
mnc_slot_t      relevance_bitmap[MAX_SUBPATH_ID][MAX_DOC_UNIQ_SYM];

/* query / document slot sub-scores */
mnc_score_t     doc_uniq_sym_score[MAX_DOC_UNIQ_SYM];

/*
 * implementation functions
 */

/* push query (should be pushed in the order of query path symbol) */
uint32_t
mnc_push_qry(struct mnc_ref qry_path_ref)
{
	qry_sym[n_qry_syms] = qry_path_ref.sym;
	qry_fnp[n_qry_syms] = qry_path_ref.fnp;
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

void mnc_doc_add_reles(uint32_t qry_path, uint64_t doc_paths,
                      struct mnc_ref doc_path_ref)
{
	/* setup doc symbolic relevance map */
	uint32_t slot = map_slot(doc_path_ref.sym);
	relevance_bitmap[qry_path][slot] |= doc_paths;

	/* save doc path fingerprint */
	// doc_fnp[doc_path] = doc_path_ref.fnp;
}

/*
 * print functions for debug
 */
static void print_slot(char *byte)
{
	int i;
	for (i = 0; i < MNC_SLOTS_BYTES; i++) {
		printf(BYTE_STR_FMT " ", BYTE_STR_ARGS(*byte));
		byte ++;
	}
}

#define _MAX_SYMBOL_STR_LEN 7

#define _PADDING_SPACE \
	if (i != n_doc_uniq_syms - 1) \
		for (j = 1; j < MNC_SLOTS_BYTES; j++) \
			printf("%*c ", 8, ' ');

static void
mnc_print(int highlight_qry_path, int max_subscore_idx)
{
	uint32_t i, j;
	if (n_doc_uniq_syms == 0)
		goto print_bitmap;

	/* print scores */
	printf("Max sub-score: %u from doc symbol `%s'\n",
	       doc_uniq_sym_score[max_subscore_idx],
	       trans_symbol(doc_uniq_sym[max_subscore_idx]));

	printf("%*s", _MAX_SYMBOL_STR_LEN, "Score: ");
	for (i = 0; i < n_doc_uniq_syms; i++) {
		printf("%-*u ", 8, doc_uniq_sym_score[i]);
		_PADDING_SPACE;
	}
	printf("\n");

	/* print document symbol slots */
	printf("%*c", _MAX_SYMBOL_STR_LEN, ' ');
	for (i = 0; i < n_doc_uniq_syms; i++) {
		printf("%-*s ", 8, trans_symbol(doc_uniq_sym[i]));
		_PADDING_SPACE;
	}
	printf("\n");

	/* print mark and cross rows */
	printf("Cross: ");
	for (i = 0; i < n_doc_uniq_syms; i++)
		print_slot((char*)&doc_cross_bitmap[i]);
	printf("\n");

	printf("Mark:  ");
	for (i = 0; i < n_doc_uniq_syms; i++)
		print_slot((char*)&doc_mark_bitmap[i]);
	printf("\n");

print_bitmap:

	/* print main bitmaps */
	for (i = 0; i < n_qry_syms; i++) {
		if ((uint32_t)highlight_qry_path == i)
			printf("-> %-*s", _MAX_SYMBOL_STR_LEN - 3,
			       trans_symbol(qry_sym[i]));
		else
			printf("%-*s", _MAX_SYMBOL_STR_LEN,
			       trans_symbol(qry_sym[i]));

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

	memset(doc_cross_bitmap, 0, sizeof(mnc_slot_t) * n_doc_uniq_syms);

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
	mnc_slot_t unmark;
	mnc_slot_t mark   = doc_mark_bitmap[j];
	mnc_slot_t cross  = doc_cross_bitmap[j];

	/* get relevance bitmap without marked or crossed bits */
	unmark = relevance_bitmap[i][j] & ~(mark | cross);

	/* no relevant bits now */
	if (unmark == 0)
		return 0;

	/* save the lowest set bit index number */
	uint32_t lo_doc_path = lsb_pos(unmark);

	/* extract the lowest set bit (only need to mark one),
	 * write this mark bit on doc_mark_bitmap */
	doc_mark_bitmap[j] |= unmark & ~(unmark - 1);

	/* return score */
	int fnp_equal = (qry_fnp[i] == doc_fnp[lo_doc_path]) ? 0x1 : 0x0;
	int sym_equal = (qry_sym[i] == doc_uniq_sym[j]) ? 0x2 : 0x0;
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

static __inline void cross(int max_slot)
{
	/* rule out the document path in the "max" slot */
	doc_cross_bitmap[max_slot] |= doc_mark_bitmap[max_slot];

	/* clear 'mark' bitmap */
	memset(doc_mark_bitmap, 0, sizeof(mnc_slot_t) * n_doc_uniq_syms);
}

mnc_score_t mnc_score(bool en_early_termination /* enable early termination */)
{
	uint32_t i, j, max_subscore_idx = 0;
	bool early_termination = false;

	mnc_score_t mark_score, total_score = 0;
	mnc_score_t max_subscore = 0;

#ifdef MNC_DEBUG
	/* print initial state */
	mnc_print(-1, max_subscore_idx);
#endif

	for (i = 0; i < n_qry_syms && !early_termination; i++) {
		early_termination = en_early_termination;
		for (j = 0; j < n_doc_uniq_syms; j++) {
			mark_score = mark(i, j);

			if (mark_score != 0) {
				early_termination = false;
				doc_uniq_sym_score[j] += mark_score;
				if (doc_uniq_sym_score[j] > max_subscore) {
					max_subscore = doc_uniq_sym_score[j];
					max_subscore_idx = j;
				}
			}
		}

		if (early_termination || /* early termination */
		    n_qry_syms == i + 1 || /* this is the final iteration */
		    qry_sym[i + 1] != qry_sym[i] /* next symbol is different */) {

#ifdef MNC_DEBUG
			/* print before cross */
			mnc_print(i, max_subscore_idx);
#endif

			cross(max_subscore_idx);

			/* accumulate into total score */
			if (early_termination)
				total_score = 0;
			else
				total_score += max_subscore;

#ifdef MNC_DEBUG
			/* print after cross */
			printf(C_RED "Current total score: %u\n" C_RST, total_score);
			if (early_termination)
				printf("(early termination).\n");
			else
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
			mnc_print(i, max_subscore_idx);
			printf("~~~~~~~~~\n");
		}
#endif
	}

	clean_bitmaps();
	return total_score;
}
