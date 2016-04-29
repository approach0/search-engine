#include <stdbool.h>
#include <stdint.h>

#define MAX_DIR_PATH_NAME_LEN 4096
#define MAX_FILE_NAME_LEN     1024

bool dir_exists(const char*);

int file_exists(const char*);

enum ds_ret {
	DS_RET_STOP_SUBDIR,
	DS_RET_STOP_ALLDIR,
	DS_RET_CONTINUE
};

typedef enum ds_ret (*ds_callbk)(const char*, const char *,
                                 uint32_t, void *);

/* recursive post-order DFS of directory */
int dir_search_podfs(const char*, ds_callbk, void *);

typedef int (*ffi_callbk)(const char*, void *);
int foreach_files_in(const char*, ffi_callbk, void*);

char *filename_ext(const char*);
