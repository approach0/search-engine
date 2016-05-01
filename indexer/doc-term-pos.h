#pragma pack(push, 1)

typedef struct {
	doc_id_t docID;
	char     term[MAX_TERM_BYTES];
} docterm_t;
	
typedef struct {
	uint32_t doc_pos, n_bytes;
} docterm_pos_t;

#pragma pack(pop)
