/* This is the bitmap-way implementation of mnc score (or "mark
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
#include "tex-parser/tex-parser.h" /* for symbol_id_t */
#include "tex-parser/vt100-color.h"
#include "config.h"
#include "mnc-score.h"

/* byte printing macros */
#define BYTE_STR_FMT "%d%d%d%d%d%d%d%d"

#define BYTE_STR_ARGS(_byte)  \
	(_byte & 0x80 ? 1 : 0), \
	(_byte & 0x40 ? 1 : 0), \
	(_byte & 0x20 ? 1 : 0), \
	(_byte & 0x10 ? 1 : 0), \
	(_byte & 0x08 ? 1 : 0), \
	(_byte & 0x04 ? 1 : 0), \
	(_byte & 0x02 ? 1 : 0), \
	(_byte & 0x01 ? 1 : 0)

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

/*
 * statical local variables
 */

/* query expression ordered subpaths/symbols list */
symbol_id_t     qry_sym[MAX_SUBPATH_ID];
uint32_t        n_qry_syms;

/* document expression unique symbols list */
symbol_id_t     doc_uniq_sym[MAX_DOC_UNIQ_SYM];
uint32_t        n_doc_uniq_syms;

/* query / document bitmaps (assume all zeros in BSS segment) */
mnc_slot_t      doc_mark_bitmap[MAX_DOC_UNIQ_SYM];
mnc_slot_t      doc_cross_bitmap[MAX_DOC_UNIQ_SYM];
mnc_slot_t      relevance_bitmap[MAX_SUBPATH_ID][MAX_DOC_UNIQ_SYM];

/* query / document slot sub-scores (assume all zero in BSS segment) */
mnc_score_t     doc_uniq_sym_score[MAX_DOC_UNIQ_SYM];

/*
 * implementation functions
 */

/* push query (should be pushed in the order of query path symbol) */
void
mnc_push_qry(struct mnc_ref qry_path_ref)
{
	qry_sym[n_qry_syms ++] = qry_path_ref.sym;
}

/* return slot index that this document path belongs */
uint32_t mnc_map_slot(struct mnc_ref doc_path_ref)
{
	uint32_t i;
	/* find the symbol slot to which doc_path belongs. */
	for (i = 0; i < n_doc_uniq_syms; i++)
		if (doc_uniq_sym[i] == doc_path_ref.sym)
			break;

	/* not found, append a new slot and add the doc_path */
	if (i == n_doc_uniq_syms) {
		doc_uniq_sym[i] = doc_path_ref.sym;
		n_doc_uniq_syms ++;
	}

	return i;
}

/* set the corresponding bit to indicate relevance between two paths */
void
mnc_doc_add_rele(uint32_t slot, uint32_t doc_path, uint32_t qry_path)
{
	mnc_slot_t bit = 1;
	bit = bit << doc_path;
	relevance_bitmap[qry_path][slot] |= bit;
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
	int i, j;
	if (n_doc_uniq_syms == 0)
		goto print_bitmap;

	/* print scores */
	printf("Max sub-score: %u from doc symbol `%c'\n",
	       doc_uniq_sym_score[max_subscore_idx],
	       doc_uniq_sym[max_subscore_idx]);

	printf("%*s", _MAX_SYMBOL_STR_LEN, "Score: ");
	for (i = 0; i < n_doc_uniq_syms; i++) {
		printf("%-*u ", 8, doc_uniq_sym_score[i]);
		_PADDING_SPACE;
	}
	printf("\n");

	/* print document symbol slots */
	printf("%*c", _MAX_SYMBOL_STR_LEN, ' ');
	for (i = 0; i < n_doc_uniq_syms; i++) {
		printf("%-*c ", 8, doc_uniq_sym[i]);
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
		if (highlight_qry_path == i)
			printf("-> %-*c", _MAX_SYMBOL_STR_LEN - 3,
			       qry_sym[i]);
		else
			printf("%-*c", _MAX_SYMBOL_STR_LEN,
			       qry_sym[i]);

		for (j = 0; j < n_doc_uniq_syms; j++)
			print_slot((char*)&relevance_bitmap[i][j]);

		printf("\n");
	}
}

/*
 * cleaning functions.
 */
void mnc_reset_dimension()
{
	/* reset dimension */
	n_qry_syms = 0;
	n_doc_uniq_syms = 0;
}

static void clean_bitmaps()
{
	/* clean bitmaps to the n_qry_syms, n_doc_uniq_syms dimension */
	int i;
	memset(doc_mark_bitmap, 0, sizeof(mnc_slot_t) * n_doc_uniq_syms);
	memset(doc_cross_bitmap, 0, sizeof(mnc_slot_t) * n_doc_uniq_syms);

	for (i = 0; i < n_qry_syms; i++)
		memset(relevance_bitmap[i], 0,
		       sizeof(mnc_slot_t) * n_doc_uniq_syms);
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

	/* extract the lowest set bit (only need to mark one),
	 * write this mark bit on doc_mark_bitmap */
	doc_mark_bitmap[j] |= unmark & ~(unmark - 1);

	/* return score */
	if (qry_sym[i] == doc_uniq_sym[j])
		return MNC_MARK_SCORE + 1; /* bonus for exact match */
	else
		return MNC_MARK_SCORE; /* normal match */
}

static __inline void cross(int j, int max_subscore_idx)
{
	if (j == max_subscore_idx)
		/* cross the document paths in the "max" slot */
		doc_cross_bitmap[j] |= doc_mark_bitmap[j];

	/* reset mark bitmap */
	doc_mark_bitmap[j] = 0;
}

mnc_score_t mnc_score()
{
	int i, j, max_subscore_idx = 0;
	bool early_termination = false;

	mnc_score_t mark_score, total_score = 0;
	mnc_score_t max_subscore = 0;

#ifdef MNC_DEBUG
	/* print initial state */
	mnc_print(-1, max_subscore_idx);
#endif

	for (i = 0; i < n_qry_syms && !early_termination; i++) {
		early_termination = true;
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

			for (j = 0; j < n_doc_uniq_syms; j++)
				cross(j, max_subscore_idx);

			/* accumulate into total score */
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
