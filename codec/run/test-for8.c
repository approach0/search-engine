#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "for8.h"

static void print_int8_arr(uint8_t *arr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		printf("%u ", arr[i]);

	printf("\n");
}

static void test(uint8_t *input_arr, size_t len)
{
	size_t res, b, i;
	uint8_t enc_arr[1024];
	uint8_t check_arr[1024];

	printf("input_arr (total %lu bytes): ",
	       len * sizeof input_arr[0]);
	print_int8_arr(input_arr, len);

	res = for8_compress(input_arr, len, enc_arr, &b);
	printf("compressed into %lu bytes, "
	       "encode using b = {{{{ %lu }}}}\n",
	       res, b);

	b = 0;
	for8_decompress(enc_arr, check_arr, len, &b);
	printf("decompressed, b = {{{{ %lu }}}}\n", b);

	printf("check_arr: ");
	print_int8_arr(check_arr, len);

	for (i = 0; i < len; i++)
		if (input_arr[i] != check_arr[i])
			assert(0);
}

int main()
{
	uint8_t input_arr[1024];
	size_t n, i, len = 20;
	srand(time(NULL));

	printf("=== test[0] ===\n");
	input_arr[0] = 0;
	input_arr[1] = 0x80;

	test(input_arr, 2);

	for (n = 1; n < 5000; n++) {
		printf("=== test[%lu] ===\n", n);
		for (i = 0; i < len; i++)
			input_arr[i] = gen_rand_int(n % 7 + 1);

		test(input_arr, len);
	}

	printf("[ pass all tests ]\n");

	mhook_print_unfree();
	return 0;
}
