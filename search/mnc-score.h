#pragma once

/* path attributes */
struct mnc_ref {
	symbol_id_t sym, fnp;
};

/* define max number of slots (bound variables) */
#define MAX_DOC_UNIQ_SYM MAX_SUBPATH_ID

#include "config.h"

/* math score value type */
typedef uint32_t mnc_score_t;

/* math score slot type */
typedef uint64_t mnc_slot_t;

/* finger print */
typedef uint16_t mnc_finpr_t;

struct mnc_match_t {
	mnc_score_t score;
	mnc_slot_t qry_paths, doc_paths;
};

void        mnc_reset_qry(void);
void        mnc_reset_docs(void);

uint32_t    mnc_push_qry(struct mnc_ref, int, uint64_t);
void        mnc_doc_add_rele(uint32_t, uint32_t, struct mnc_ref);
void        mnc_doc_add_reles(uint32_t, mnc_slot_t, struct mnc_ref);

int         lsb_pos(uint64_t);

struct mnc_match_t mnc_match(void);
struct mnc_match_t mnc_match_debug(void);

void mnc_print(mnc_score_t*, uint64_t, int, int);
