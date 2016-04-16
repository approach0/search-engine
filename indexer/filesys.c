#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "filesys.h"
#include "config.h"

bool dir_exists(const char *path)
{
	struct stat s;
	stat(path, &s);
	return S_ISDIR(s.st_mode);
}

int file_exists(const char *path)
{
	struct stat s;
	stat(path, &s);
	return S_ISREG(s.st_mode);
}

static enum ds_ret
_dir_search_podfs(const char *path, const char *srchpath, uint32_t level,
                  ds_callbk callbk_fun, char *arg)
{
	struct dirent *dent;
	char *dname;
	DIR  *dir;
	enum ds_ret ret, res;
	char newpath[2][MAX_DIR_PATH_NAME_LEN];

	dir = opendir(path);
	if (dir == NULL)
		return 1;

	ret = callbk_fun(path, srchpath, level, arg);
	if (ret == DS_RET_STOP_SUBDIR)
		goto exit;
	else if (ret == DS_RET_STOP_ALLDIR)
		goto exit;

	while (1) {
		dent = readdir(dir);
		if (!dent)
			return DS_RET_CONTINUE;

		dname = dent->d_name;
		if (dent->d_type & DT_DIR) {
			if (strcmp(dname, "..") == 0 ||
			    strcmp(dname, ".") == 0)
				continue;

			sprintf(newpath[0], "%s/%s", path, dname);
			sprintf(newpath[1], "%s/%s", srchpath, dname);
			res = _dir_search_podfs(newpath[0], newpath[1],
			                        level + 1, callbk_fun, arg);
			if (res == DS_RET_STOP_ALLDIR) {
				ret = res;
				break;
			}
		}
	}

exit:
	closedir(dir);
	return ret;
}

int
dir_search_podfs(const char *path, ds_callbk callbk_fun, void *arg)
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
	                                            0, callbk_fun, arg))
		return 1;
	else
		return 0;
}
