/* factors in subpath that contribute to math score */
struct mnc_ref {
	symbol_id_t sym, fr;
};

/* math score value type */
typedef uint32_t mnc_score_t;

void        mnc_reset_dimension(void);
void        mnc_push_qry(struct mnc_ref);
uint32_t    mnc_map_slot(struct mnc_ref);
void        mnc_doc_add_rele(uint32_t, uint32_t, uint32_t);
mnc_score_t mnc_score(void);
int         lsb_pos(uint64_t);
