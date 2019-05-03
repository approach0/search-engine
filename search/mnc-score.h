#pragma once

#include "postmerge/config.h"
#include "config.h"

/* define max number of query paths */
#define MAX_MNC_QRY_PATHS MAX_MERGE_POSTINGS

/* define max number of slots (bound variables) */
#define MAX_DOC_UNIQ_SYM MAX_SUBPATH_ID

/* path attributes */
struct mnc_ref {
	symbol_id_t sym, fnp;
};

/* math score value type */
typedef uint32_t mnc_score_t;

/* math score slot type */
typedef uint64_t mnc_slot_t;

/* finger print */
typedef uint16_t mnc_finpr_t;

typedef struct mnc {
	/* query expression ordered subpaths/symbols list */
	symbol_id_t     qry_sym[MAX_MNC_QRY_PATHS];
	mnc_finpr_t     qry_fnp[MAX_MNC_QRY_PATHS];
	int             qry_wil[MAX_MNC_QRY_PATHS];
	uint64_t        qry_cnj[MAX_MNC_QRY_PATHS];
	uint32_t        n_qry_syms;

	/* document expression unique symbols list */
	symbol_id_t     doc_uniq_sym[MAX_DOC_UNIQ_SYM];
	mnc_finpr_t     doc_fnp[MAX_SUBPATH_ID];
	mnc_finpr_t     doc_wild_fnp[MAX_DOC_UNIQ_SYM];
	uint32_t        n_doc_uniq_syms;

	/* query / document bitmaps */
	mnc_slot_t      doc_mark_bitmap[MAX_DOC_UNIQ_SYM];
	mnc_slot_t      doc_cross_bitmap;
	mnc_slot_t      relevance_bitmap[MAX_MNC_QRY_PATHS][MAX_DOC_UNIQ_SYM];
} *mnc_ptr_t;

struct mnc_match_t {
	mnc_score_t score;
	mnc_slot_t qry_paths, doc_paths;
};

mnc_ptr_t mnc_alloc();
void      mnc_free(mnc_ptr_t);
void      mnc_reset_docs(mnc_ptr_t);

uint32_t  mnc_push_qry(mnc_ptr_t, struct mnc_ref, int, uint64_t);
void      mnc_doc_add_rele(mnc_ptr_t, uint32_t, uint32_t, struct mnc_ref);
void      mnc_doc_add_reles(mnc_ptr_t, uint32_t, mnc_slot_t, struct mnc_ref);

struct mnc_match_t mnc_match(mnc_ptr_t);
struct mnc_match_t mnc_match_debug(mnc_ptr_t);;
