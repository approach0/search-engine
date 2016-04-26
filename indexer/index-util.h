struct splited_term {
	char             term[MAX_TERM_BYTES];
	uint32_t         doc_pos /* in bytes */, n_bytes;
	struct list_node ln;
};

void split_into_terms(struct lex_slice*, list *);
void release_splited_terms(list *);

void eng_to_lower_case(struct lex_slice*);

void lex_txt_file(const char*);

extern uint32_t file_pos_check_cnt;
int  file_pos_check_init(const char*);
void file_pos_check_free(void);
void file_pos_check_add(size_t, size_t);
void file_pos_check_print(void);
