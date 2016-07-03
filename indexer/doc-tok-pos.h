#pragma once

#pragma pack(push, 1)

typedef struct {
	doc_id_t docID;
	char     term[MAX_TERM_BYTES];
} docterm_t;

typedef uint32_t exp_id_t;

typedef struct {
	doc_id_t docID;
	exp_id_t expID;
} doctex_t;

typedef struct {
	uint32_t doc_pos, n_bytes;
} doctok_pos_t;

#pragma pack(pop)
