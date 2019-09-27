#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
 * allocation / cleaning functions.
 */

mnc_ptr_t mnc_alloc()
{
	return calloc(1, sizeof(struct mnc));
}

void mnc_free(mnc_ptr_t p)
{
	free(p);
}

/* reset document */
void mnc_reset_docs(mnc_ptr_t p)
{
	p->n_doc_uniq_syms = 0;
}

/* clean bitmaps to the n_qry_syms, n_doc_uniq_syms dimension */
static void clear_bitmaps(mnc_ptr_t p)
{
	uint32_t i;

	/* No need to clear 'mark bitmap' because cross() function
	 * already ensures a clean 'mark bitmap' after main function. */

	/* clear 'cross bitmap' */
	p->doc_cross_bitmap = 0x0;

	/* clear relevance bitmap */
	for (i = 0; i < p->n_qry_syms; i++) {
		memset(p->relevance_bitmap[i], 0,
		       sizeof(mnc_slot_t) * p->n_doc_uniq_syms);
	}
}

/*
 * implementation functions
 */

/* push query (should be pushed in the order of query path symbol) */
uint32_t
mnc_push_qry(mnc_ptr_t p, struct mnc_ref qry_path_ref,
             int is_wildcard, uint64_t conjugacy)
{
	p->qry_sym[p->n_qry_syms] = qry_path_ref.sym;
	p->qry_fnp[p->n_qry_syms] = qry_path_ref.fnp;
	p->qry_wil[p->n_qry_syms] = is_wildcard;
	p->qry_cnj[p->n_qry_syms] = conjugacy;
	return (p->n_qry_syms ++);
}

/* return slot index that this document path belongs */
static uint32_t map_slot(mnc_ptr_t p, symbol_id_t doc_sym)
{
	uint32_t i;
	/* find the symbol slot to which doc_path belongs. */
	for (i = 0; i < p->n_doc_uniq_syms; i++)
		if (p->doc_uniq_sym[i] == doc_sym)
			break;

	/* not found, append a new slot and add the doc_path */
	if (i == p->n_doc_uniq_syms) {
		p->doc_uniq_sym[i] = doc_sym;
		p->n_doc_uniq_syms ++;
	}

	return i;
}

/* set the corresponding bit to indicate relevance between two paths */
void
mnc_doc_add_rele(mnc_ptr_t p, uint32_t qry_path,
	uint32_t doc_path, struct mnc_ref doc_path_ref)
{
	/* setup doc symbolic relevance map */
	uint32_t slot = map_slot(p, doc_path_ref.sym);
	mnc_slot_t bit = 1;
	bit = bit << doc_path;
	p->relevance_bitmap[qry_path][slot] |= bit;

	/* save doc path fingerprint */
	p->doc_fnp[doc_path] = doc_path_ref.fnp;
}

/* set the corresponding bits to indicate relevance between
 * a wildcard query path and a document gener path. */
void
mnc_doc_add_reles(mnc_ptr_t p, uint32_t qry_path,
	mnc_slot_t doc_paths, struct mnc_ref doc_path_ref)
{
	/* setup doc symbolic relevance map */
	uint32_t slot = map_slot(p, doc_path_ref.sym);
	p->relevance_bitmap[qry_path][slot] |= doc_paths;

	/* save gener path fingerprint by slot */
	p->doc_wild_fnp[slot] = doc_path_ref.fnp;
}

/*
 * print functions for debug
 */
#define MAX_SYMSTR_LEN 16

#define _MARGIN_PAD printf(" ")

static void print_slot(unsigned char *byte)
{
	uint64_t u64 = *(uint64_t*)byte;
	for (int i = 7; i >= 0; i--) {
		if (u64 == 0) printf(C_GRAY);
		printf("%02x", byte[i]);
		if (u64 == 0) printf(C_RST);
	}
	_MARGIN_PAD;
}

static void mnc_print(mnc_ptr_t p,mnc_score_t *sub_score,
	uint64_t conjugacy, int highlight_qry_path, int max_subscore_idx)
{
	uint32_t i, j;
	if (p->n_doc_uniq_syms == 0)
		goto print_bitmap;

	/* print scores */
	printf("Max sub-score: %u from doc symbol `%s'\n",
	       sub_score[max_subscore_idx],
	       trans_symbol_wo_font(p->doc_uniq_sym[max_subscore_idx]));

	printf("%-*s", MAX_SYMSTR_LEN, "Score: ");
	for (i = 0; i < p->n_doc_uniq_syms; i++) {
		printf("%-*u ", MAX_SYMSTR_LEN, sub_score[i]);
		_MARGIN_PAD;
	}
	printf("\n");

	/* print document symbol slots */
	printf("%*c", MAX_SYMSTR_LEN, ' ');
	for (i = 0; i < p->n_doc_uniq_syms; i++) {
		printf("%-*s ", MAX_SYMSTR_LEN,
			trans_symbol_wo_font(p->doc_uniq_sym[i]));
		_MARGIN_PAD;
	}
	printf("\n");

	/* print mark and cross rows */
	printf("Cross:");
	print_slot((uint8_t*)&p->doc_cross_bitmap);
	printf("   ");
	printf("Conjugacy:");
	print_slot((uint8_t*)&conjugacy);
	printf("\n");

	printf("%-*s", MAX_SYMSTR_LEN, "Mark: ");
	for (i = 0; i < p->n_doc_uniq_syms; i++)
		print_slot((uint8_t*)&p->doc_mark_bitmap[i]);
	printf("\n");

print_bitmap:

	/* print main bitmaps */
	for (i = 0; i < p->n_qry_syms; i++) {
		if (highlight_qry_path == -1 || highlight_qry_path == (int)i)
			printf("[%d]-> %-*s", p->qry_wil[i], MAX_SYMSTR_LEN - 3 - 2 - 1,
			       trans_symbol_wo_font(p->qry_sym[i]));
		else
			printf("[%d] %-*s", p->qry_wil[i], MAX_SYMSTR_LEN - 3 - 1,
			       trans_symbol_wo_font(p->qry_sym[i]));

		for (j = 0; j < p->n_doc_uniq_syms; j++)
			print_slot((uint8_t*)&p->relevance_bitmap[i][j]);

		printf("\n");
	}
}

/* return the index of least significant bit position */
static inline int lsb_pos(uint64_t v)
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
static __inline mnc_score_t mark(mnc_ptr_t p, int i, int j)
{
	mnc_slot_t mark  = p->doc_mark_bitmap[j];
	mnc_slot_t cross = p->doc_cross_bitmap;
	mnc_slot_t rels = p->relevance_bitmap[i][j];

	/* get relevance bitmap without conflict bits */
	mnc_slot_t unmark = rels & ~(mark | cross);

	/* if there is no relevant bits left ... */
	if (unmark == 0)
		return 0;

	int fnp_equal;

	if (p->qry_wil[i]) {
		/* wildcard path */

		if (unmark == rels) {
			/* no conflict at all */

			/* mark all relevance bits */
			p->doc_mark_bitmap[j] |= rels;
		} else {
			/* won't match on any conflict */
			return 0;
		}

		/* wildcard match fingerprint score */
		if (p->qry_fnp[i] == p->doc_wild_fnp[j])
			fnp_equal = 0x1;
		else
			fnp_equal = 0x0;

	} else {
		/* concrete path */

		/* extract the lowest set bit (only need to mark one),
		 * set this mark bit on doc_mark_bitmap */
		p->doc_mark_bitmap[j] |= unmark & ~(unmark - 1);

		/* fingerpirnt score */
		uint32_t lo_doc_path = lsb_pos(unmark);
		if (p->qry_fnp[i] == p->doc_fnp[lo_doc_path])
			fnp_equal = 0x1;
		else
			fnp_equal = 0x0;
	}

	/* symbol score */
	int sym_equal = (p->qry_sym[i] == p->doc_uniq_sym[j]) ? 0x2 : 0x0;

	/* return score */
	switch (sym_equal | fnp_equal) {
	case 0x3:
		return MNC_MARK_FULL_SCORE; /* exact match */
		break;
	case 0x2:
		return MNC_MARK_MID_SCORE;  /* match except for fingerprint */
		break;
	case 0x1:
		return MNC_MARK_BASE_SCORE;  /* match fingerprint but not symbol */
		break;
	default:
		return MNC_MARK_BASE_SCORE;
	}
}

static __inline mnc_slot_t cross(mnc_ptr_t p, int max_slot)
{
	/* rule out the document path in the "max" slot */
	p->doc_cross_bitmap |= p->doc_mark_bitmap[max_slot];

	/* clear 'mark' bitmap */
	memset(p->doc_mark_bitmap, 0, sizeof(mnc_slot_t) * p->n_doc_uniq_syms);

	return p->doc_cross_bitmap;
}

struct mnc_match_t mnc_match(mnc_ptr_t p)
{
	struct mnc_match_t ret;
	uint32_t i, j, max_subscore_idx = 0;
	mnc_slot_t qry_cross_bitmap = 0;
	uint64_t conjugacy = 0L;

	mnc_score_t mark_score, total_score = 0;
	mnc_score_t max_subscore = 0;

	/* query / document slot sub-scores */
	mnc_score_t doc_uniq_sym_score[MAX_DOC_UNIQ_SYM] = {0};

#ifdef MNC_DEBUG
	/* print initial state */
	mnc_print(p, doc_uniq_sym_score, conjugacy, -1, max_subscore_idx);
#endif

	for (i = 0; i < p->n_qry_syms; i++) {
		/* if this query path is an copy resulted from query expansion,
		 * then watch out to avoid if it has previously matched ones. */
		if (conjugacy & (1L << i)) {
#ifdef MNC_DEBUG
	printf(C_CYAN "conjugacy conflict!\n" C_RST);
#endif
			goto skip_mark;
		}

		for (j = 0; j < p->n_doc_uniq_syms; j++) {

			mark_score = mark(p, i, j);

			if (mark_score != 0) {
				qry_cross_bitmap |= (1L << i); /* mask this query path */
				conjugacy |= p->qry_cnj[i]; /* mask to avoid path copies. */
				doc_uniq_sym_score[j] += mark_score;
				if (doc_uniq_sym_score[j] > max_subscore) {
					max_subscore = doc_uniq_sym_score[j];
					max_subscore_idx = j;
				}
			}
		}

skip_mark:

		if (p->n_qry_syms == i + 1 || /* this is the final iteration */
		    p->qry_sym[i + 1] != p->qry_sym[i] /* next symbol is different */) {
#ifdef MNC_DEBUG
			/* print before cross */
			mnc_print(p, doc_uniq_sym_score, conjugacy, i, max_subscore_idx);
#endif

			if (cross(p, max_subscore_idx) == (0L - 1))
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
			       sizeof(mnc_score_t) * p->n_doc_uniq_syms);
			max_subscore = 0;
			max_subscore_idx = 0;
		}
#ifdef MNC_DEBUG
		else {
			/* print */
			mnc_print(p, doc_uniq_sym_score, conjugacy, i, max_subscore_idx);
			printf("~~~~~~~~~\n");
		}
#endif
	}

	/* save return values before clean_bitmaps() */
	ret = ((struct mnc_match_t) {
		total_score,
		qry_cross_bitmap, p->doc_cross_bitmap
	});

	clear_bitmaps(p);
	return ret;
}

#define MNC_DEBUG
struct mnc_match_t mnc_match_debug(mnc_ptr_t p)
{
	return mnc_match(p);
}
