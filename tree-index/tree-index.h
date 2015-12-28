#ifndef TREE_INDEX_H
#define TREE_INDEX_H

#include "../include/config.h"
#include "../list/list.h"

struct dir_path_item {
	char             name[MAX_TR_INDEX_PATH_LEN];
	struct list_node ln;
};

struct level_postinglist_item {
	FILE             *fh; /* posting list FILE handle */
	struct list_node  ln;
};

enum level_merge_ret_t {
	LEVEL_MERGE_CONTINUE,
	LEVEL_MERGE_STOP
};

enum tree_index_open_option {
	ONLY_READ,
	WRITE_APPEND	
};

typedef enum level_merge_ret_t 
(*level_merge)(list level_postinglists, void *args);

void *
tree_index_open(const char *index_path, 
                enum tree_index_open_option option);

FILE *
tree_index_map_postinglist(void *tr_index, char *dir_path);

int
tree_index_level_merge(void *tr_index, list dir_path_list,
                       level_merge callbk, void *args);

void 
tree_index_close(void *tr_index);

#endif
