#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mhook/mhook.h"
#include "u16-ht.h"

static void test(struct u16_ht *ht)
{
	int stack[128], len = 0;
	for (int i = 0; i < 40; i++) {
		int key = random() % 32;
		printf("update <%u>\n", key);
		stack[len++] = key;
		u16_ht_incr(ht, key, 1);
		u16_ht_print(ht);
		printf("\n");
	}

	for (int i = 0; i < len; i++) {
		int key = stack[i];
		int val = u16_ht_lookup(ht, key);
		printf("lookup <%d: %d>\n", key, val);
	}

	for (int i = 0; i < 10; i++) {
		int key = random() % 32;
		int val = u16_ht_lookup(ht, key);
		printf("lookup <%d: %d>\n", key, val);
	}
}

int main()
{
	struct u16_ht ht = u16_ht_new(0);
	srand(time(NULL));

	printf("=== test 1 ===\n");
	test(&ht);
	u16_ht_reset(&ht, 0);
	printf("\n");

	printf("=== test 2 ===\n");
	test(&ht);
	u16_ht_reset(&ht, 0);
	printf("\n");

	printf("=== test 3 ===\n");
	test(&ht);
	u16_ht_reset(&ht, 0);
	printf("\n");

	u16_ht_free(&ht);
	mhook_print_unfree();
	return 0;
}
