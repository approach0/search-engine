#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "mhook/mhook.h"
#include "for.h"

void print_int32_arr(uint32_t *arr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		printf("%u ", arr[i]);

	printf("\n");
}

void test(uint32_t *input_arr, size_t len)
{
	size_t res, b, i;
	uint32_t enc_arr[1024];
	uint32_t check_arr[1024];

	printf("input_arr (total %lu bytes): ",
	       len * sizeof(uint32_t));
	print_int32_arr(input_arr, len);

	res = for_compress(input_arr, len, enc_arr, &b);
	printf("compressed into %lu bytes, "
	       "encode using b = {{{{ %lu }}}}\n",
	       res, b);

	printf("enc_arr: ");
	print_int32_arr(enc_arr, len);

	b = 0;
	for_decompress(enc_arr, check_arr, len, &b);
	printf("decompressed, b = {{{{ %lu }}}}\n", b);

	printf("check_arr: ");
	print_int32_arr(check_arr, len);

	for (i = 0; i < len; i++)
		if (input_arr[i] != check_arr[i])
			assert(0);
}

int main()
{
	uint32_t input_arr[1024];
	size_t n, i, deviation, len = 20;

	printf("=== test[0] ===\n");
	input_arr[0] = 0;
	input_arr[1] = 0x80000000;

	test(input_arr, 2);

	for (n = 1; n < 50; n++) {
		printf("=== test[%lu] ===\n", n);
		srand(time(NULL));
		deviation = n * n * n + 1 /* add one to ensure positive */;
		printf("deviation: %lu\n", deviation);
		for (i = 0; i < len; i++)
			input_arr[i] = rand() % deviation;

		test(input_arr, len);
		sleep(1);
	}

	printf("[ pass all tests ]\n");

	mhook_print_unfree();
	return 0;
}
