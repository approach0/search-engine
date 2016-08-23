#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

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

int
dir_search_podfs(const char *path, ds_callbk fun, void *arg)
{
	char legal_path[MAX_DIR_PATH_NAME_LEN];
	const char *use_path = path;
	uint32_t len = strlen(path);
	const char srchpath[] = ".";

	/* remove trailing slash */
	if (path[len - 1] == '/') {
		strcpy(legal_path, path);
		legal_path[len - 1] = '\0';
		use_path = (const char*)legal_path;
	}

	if (DS_RET_STOP_ALLDIR == _dir_search_podfs(use_path, srchpath,
	                                            0, fun, arg))
		return 1;
	else
		return 0;
}
