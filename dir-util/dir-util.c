#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdbool.h>

#include "list/list.h"
#include "dir-util.h"

bool dir_exists(const char *path)
{
	struct stat s;
	if (0 == stat(path, &s))
		return S_ISDIR(s.st_mode);
	else if (ENOENT == errno)
		return 0;
	else
		assert(0);
}

int file_exists(const char *path)
{
	struct stat s;
	if (0 == stat(path, &s))
		return S_ISREG(s.st_mode);
	else if (ENOENT == errno)
		return 0;
	else
		assert(0);
}

char *filename_ext(const char *name)
{
	static char buf[MAX_FILE_NAME_LEN];
	uint32_t i, len = strlen(name);
	strcpy(buf, name);

	for (i = len; i != 0; i--) {
		if (buf[i] == '.')
			return buf + i;
	}

	return NULL;
}

void mkdir_p(const char *path)
{
	char dir[MAX_DIR_PATH_NAME_LEN];
	char *p = NULL;
	size_t len;

	sprintf(dir, "%s", path);
	len = strlen(dir);

	if(dir[len - 1] == '/')
		dir[len - 1] = '\0';

	for(p = dir + 1; *p; p++)
		if(*p == '/') {
			*p = '\0';
			mkdir(dir, S_IRWXU);
			*p = '/';
		}

	mkdir(dir, S_IRWXU);
}

int foreach_files_in(const char *path, ffi_callbk fun, void *arg)
{
	char *fname;
	struct dirent *dent;
	char newpath[MAX_DIR_PATH_NAME_LEN];
	DIR  *dir = opendir(path);

	if (dir == NULL)
		return 1;

	while (1) {
		dent = readdir(dir);
		if (!dent)
			break;

		fname = dent->d_name;

		if (fname[0] == '.')
			/* not .. or . or hidden files*/
			continue;

		sprintf(newpath, "%s/%s", path, fname);

		/* is a regular file ? */
		if (!file_exists(newpath))
			continue;

		if (fun(fname, arg))
			break;
	}

	closedir(dir);
	return 0;
}

static enum ds_ret
_dir_search_podfs(const char *path, const char *srchpath, uint32_t level,
                  ds_callbk fun, char *arg)
{
	struct dirent *dent;
	char *dname;
	DIR  *dir;
	enum ds_ret ret, res;
	char newpath[2][MAX_DIR_PATH_NAME_LEN];

	dir = opendir(path);
	if (dir == NULL)
		return 1;

	ret = fun(path, srchpath, level, arg);
	if (ret == DS_RET_STOP_SUBDIR)
		goto exit;
	else if (ret == DS_RET_STOP_ALLDIR)
		goto exit;

	while (1) {
		dent = readdir(dir);
		if (!dent) {
			ret = DS_RET_CONTINUE;
			break;
		}

		dname = dent->d_name;

		if (dname[0] == '.')
			/* not .. or . or hidden files*/
			continue;

		sprintf(newpath[0], "%s/%s", path, dname);

		/* is a directory ? */
		if (!dir_exists(newpath[0]))
			continue;

		sprintf(newpath[1], "%s/%s", srchpath, dname);

		res = _dir_search_podfs(newpath[0], newpath[1],
		                        level + 1, fun, arg);
		if (res == DS_RET_STOP_ALLDIR) {
			ret = res;
			break;
		}
	}

exit:
	closedir(dir);
	return ret;
}

static char *rm_trailing_slash(const char *path)
{
	static char ret_path[MAX_DIR_PATH_NAME_LEN];
	uint32_t len = strlen(path);

	if (len == 0)
		return NULL;

	strcpy(ret_path, path);

	/* remove trailing slash */
	if (path[len - 1] == '/') {
		ret_path[len - 1] = '\0';
	}

	return ret_path;
}

int
dir_search_podfs(const char *path_, ds_callbk fun, void *arg)
{
	const char *path = rm_trailing_slash(path_);
	const char srchpath[] = ".";

	if (DS_RET_STOP_ALLDIR == _dir_search_podfs(path, srchpath,
	                                            0, fun, arg))
		return 1;
	else
		return 0;
}

/*
 * BFS search of directory
 */
struct Q_ele
{
	char path[MAX_DIR_PATH_NAME_LEN];
	char srchpath[MAX_DIR_PATH_NAME_LEN];
	uint32_t level;

	struct list_node ln;
};

static void
Q_push(list *Q, char *path, char *srchpath, uint32_t level)
{
	struct Q_ele *ele = malloc(sizeof(struct Q_ele));
	strcpy(ele->path, path);
	strcpy(ele->srchpath, srchpath);
	ele->level = level;

	LIST_NODE_CONS(ele->ln);
	list_insert_one_at_tail(&ele->ln, Q, NULL, NULL);
}

LIST_DEF_FREE_FUN(Q_release, struct Q_ele, ln, free(p));

static struct Q_ele *Q_pop(list *Q)
{
	struct list_node *top_node = Q->now;

	if (top_node != NULL)
		list_detach_one(top_node, Q, NULL, NULL);

	return MEMBER_2_STRUCT(top_node, struct Q_ele, ln);
}

int
dir_search_bfs(const char *path_, ds_callbk fun, void *arg)
{
	char *path      = rm_trailing_slash(path_);
	char srchpath[] = ".";

	bool break_loop = 0;
	struct Q_ele *top;
	int ret = 0;

	/* push initial element to an empty queue */
	list Q = LIST_NULL;
	Q_push(&Q, path, srchpath, 0);

	while (!break_loop && NULL != (top = Q_pop(&Q))) {
		DIR  *dir;
		enum ds_ret res;

		/* open current directory */
		dir = opendir(top->path);
		if (dir == NULL) {
			//fprintf(stderr, "BFS open err: %s.\n", top->path);

			free(top);
			break;
		}

		/* invoke callback function */
		res = fun(top->path, top->srchpath, top->level, arg);

		if (res == DS_RET_STOP_SUBDIR) {
			goto loop_end;
		} else if (res == DS_RET_STOP_ALLDIR) {
			ret = 1;
			break_loop = true;
			goto loop_end;
		}

		/* push sub-directories */
		while (1) {
			struct dirent *dent;
			char *dname;
			char newpath[2][MAX_DIR_PATH_NAME_LEN];

			dent = readdir(dir);
			if (!dent) break; /* no more sub-dirs */

			dname = dent->d_name;

			if (dname[0] == '.')
				/* not .. or . or hidden files*/
				continue;

			sprintf(newpath[0], "%s/%s", top->path, dname);

			/* is a directory ? */
			if (!dir_exists(newpath[0]))
				continue;

			sprintf(newpath[1], "%s/%s", top->srchpath, dname);
			Q_push(&Q, newpath[0], newpath[1], top->level + 1);
		}

loop_end:
		closedir(dir);
		free(top);
	}

	Q_release(&Q);

	return ret;
}
