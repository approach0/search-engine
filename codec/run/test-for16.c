#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "for16.h"

static void print_int16_arr(uint16_t *arr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		printf("%u ", arr[i]);

	printf("\n");
}

static void test(uint16_t *input_arr, size_t len)
{
	size_t res, b, i;
	uint16_t enc_arr[1024];
	uint16_t check_arr[1024];

	printf("input_arr (total %lu bytes): ",
	       len * sizeof input_arr[0]);
	print_int16_arr(input_arr, len);

	res = for16_compress(input_arr, len, enc_arr, &b);
	printf("compressed into %lu bytes, "
	       "encode using b = {{{{ %lu }}}}\n",
	       res, b);

	b = 0;
	for16_decompress(enc_arr, check_arr, len, &b);
	printf("decompressed, b = {{{{ %lu }}}}\n", b);

	printf("check_arr: ");
	print_int16_arr(check_arr, len);

	for (i = 0; i < len; i++)
		if (input_arr[i] != check_arr[i]) {
			printf("%u != %u\n", input_arr[i], check_arr[i]);
			assert(0);
		}
}

int main()
{
	uint16_t input_arr[1024];
	size_t n, i, len = 20;
	srand(time(NULL));

	printf("=== test[0] ===\n");
	input_arr[0] = 0;
	input_arr[1] = 0x8000;

	test(input_arr, 2);

	for (n = 1; n < 5000; n++) {
		printf("=== test[%lu] ===\n", n);
		for (i = 0; i < len; i++)
			input_arr[i] = gen_rand_int(n % 15 + 1);

		test(input_arr, len);
	}

	printf("[ pass all tests ]\n");

	mhook_print_unfree();
	return 0;
}
