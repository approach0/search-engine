#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main()
{
	for (float i = 4.f;; i++) {
		float sz = powf(2.f, i);
		if (sz > 65536.f) break;
		printf("{%.0f, %.0f}, \n", sz, floorf(sz * 0.75));
	}

	return 0;
}
