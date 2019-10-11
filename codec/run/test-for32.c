#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "for32.h"

static void print_int32_arr(uint32_t *arr, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++)
		printf("%u ", arr[i]);

	printf("\n");
}

static void test(uint32_t *input_arr, size_t len)
{
	size_t res, b, i;
	uint32_t enc_arr[1024];
	uint32_t check_arr[1024];

	printf("input_arr (total %lu bytes): ",
	       len * sizeof(uint32_t));
	print_int32_arr(input_arr, len);

	res = for32_compress(input_arr, len, enc_arr, &b);
	printf("compressed into %lu bytes, "
	       "encode using b = {{{{ %lu }}}}\n",
	       res, b);

	printf("enc_arr: ");
	print_int32_arr(enc_arr, len);

	b = 0;
	for32_decompress(enc_arr, check_arr, len, &b);
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
	size_t n, i, len = 20;
	srand(time(NULL));

	printf("=== test[0] ===\n");
	input_arr[0] = 0;
	input_arr[1] = 0x80000000;

	test(input_arr, 2);

	for (n = 1; n < 5000; n++) {
		printf("=== test[%lu] ===\n", n);
		for (i = 0; i < len; i++)
			input_arr[i] = gen_rand_int(n % 31 + 1);

		test(input_arr, len);
	}

	printf("[ pass all tests ]\n");

	mhook_print_unfree();
	return 0;
}
