#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

int flock_exlock(const char *path)
{
	int fd = open(path, O_WRONLY | O_CREAT, 0600);

	if (fd != -1) {
		/* flock() w/o LOCK_NB is a blocking call */
		if (0 != flock(fd, LOCK_EX)) {
			fprintf(stderr, "flock() failed unexpectedly.\n");
		}
	} else {
		fprintf(stderr, "cannot open %s.\n", path);
	}

	return fd;
}

int flock_unlock(int fd)
{
	if (fd == -1)
		return 1;

	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}
