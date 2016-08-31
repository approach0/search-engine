#include <stdio.h>
#include "timer.h"

void delay(time_t sec, long ms, long us)
{
	struct timespec interval;
	interval.tv_sec = sec;
	interval.tv_nsec = us * 1000 + ms * 1000000;

	clock_nanosleep(CLOCK_MONOTONIC, 0, &interval, NULL);
}

void timer_reset(struct timer *t)
{
	struct timespec cur;
	clock_gettime(CLOCK_MONOTONIC, &cur);
	t->begin_msec = cur.tv_sec * 1000 + cur.tv_nsec / 1000000;
	t->last_msec = t->begin_msec;
}

long timer_tot_msec(struct timer *t)
{
	struct timespec cur;
	clock_gettime(CLOCK_MONOTONIC, &cur);
	t->last_msec = cur.tv_sec * 1000 + cur.tv_nsec / 1000000;
	return t->last_msec - t->begin_msec;
}

long timer_last_msec(struct timer *t)
{
	struct timespec cur;
	long cur_msec, ret_msec;
	clock_gettime(CLOCK_MONOTONIC, &cur);
	cur_msec = cur.tv_sec * 1000 + cur.tv_nsec / 1000000;
	ret_msec = cur_msec - t->last_msec;
	t->last_msec = cur_msec;

	return ret_msec;
}
