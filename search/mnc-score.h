#pragma once

/* path attributes */
struct mnc_ref {
	symbol_id_t sym, fnp;
};

/* math score value type */
typedef uint32_t mnc_score_t;

void        mnc_reset_qry(void);
void        mnc_reset_docs(void);

uint32_t    mnc_push_qry(struct mnc_ref, int);
void        mnc_doc_add_rele(uint32_t, uint32_t, struct mnc_ref);
void        mnc_doc_add_reles(uint32_t, uint64_t, struct mnc_ref);

int         lsb_pos(uint64_t);

/* define max number of slots (bound variables) */
#define MAX_DOC_UNIQ_SYM MAX_SUBPATH_ID

/* define wildcard fingerprint magic number */
#define WILDCARD_FINGERPRINT_MAGIC 0x1234

#include "config.h"

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

struct mnc_match_t {
	mnc_score_t score;
	mnc_slot_t qry_paths, doc_paths;
};

struct mnc_match_t mnc_match(void);
