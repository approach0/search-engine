#include <stdio.h>
#include <unistd.h>

#include "flock.h"

int main()
{
	const char path[] = "./test.lock";
	printf("obtaining lock for %s ...\n", path);
	int lock_fd = flock_exlock(path);
	printf("obtained.\n");
	sleep(5);

	flock_unlock(lock_fd);
	printf("released.\n");

	return 0;
}
