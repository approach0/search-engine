#pragma once

#define WILDCARD_FINGERPRINT_MAGIC 0x1234

/* factors in subpath that contribute to math score */
struct mnc_ref {
	symbol_id_t sym, fnp;
};

/* math score value type */
typedef uint32_t mnc_score_t;

void        mnc_reset_qry(void);
void        mnc_reset_docs(void);

uint32_t    mnc_push_qry(struct mnc_ref);
void        mnc_doc_add_rele(uint32_t, uint32_t, struct mnc_ref);
void        mnc_doc_add_reles(uint32_t, uint64_t, struct mnc_ref);
mnc_score_t mnc_score(void);

int         lsb_pos(uint64_t);
