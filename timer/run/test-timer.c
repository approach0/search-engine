#include <stdio.h>
#include "timer.h"

int main()
{
	struct timer t;

	timer_reset(&t);
	delay(1, 500, 0);
	printf("%ld msec.\n", timer_last_msec(&t));
	delay(1, 500, 0);
	printf("%ld msec.\n", timer_last_msec(&t));
	delay(1, 500, 0);
	printf("%ld msec.\n", timer_tot_msec(&t));

	return 0;
}
