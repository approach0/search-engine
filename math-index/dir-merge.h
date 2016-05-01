enum dir_merge_ret {
	DIR_MERGE_CONTINUE,
	DIR_MERGE_STOP
};

enum dir_merge_type {
	DIR_MERGE_DEPTH_FIRST,
	DIR_MERGE_BREADTH_FIRST
};

typedef enum dir_merge_ret
(*dir_merge_callbk)(math_posting_t [MAX_MATH_PATHS],
                    uint32_t /* number of postings list */,
                    uint32_t /* level */, void *args);

int math_index_dir_merge_df(math_index_t, enum dir_merge_type,
                            struct subpaths, dir_merge_callbk,
                            void *args);
