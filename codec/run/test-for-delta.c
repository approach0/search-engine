#include <stdio.h>
#include "codec.h"
#include <stdlib.h>
#include <time.h>

int main()
{
	size_t j, i, res;
#if 1
	#define len 10
#else
	#define len 1024
#endif
	uint32_t input_arr[len];
	uint32_t delta_arr[len - 1];
	uint32_t enc_arr[
		len + 1 /* FOR-delta can become larger because of its header */
		/* e.g. sequence 0, 0x80000000 will have one byte b header, one 32 bits
		 * begining integer header and one 32 bits delta encoded value */
	];
	uint32_t check_arr[len] = {0};

	struct codec *codec = codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS);

	// define a array to test each case for different b
	int test_max[8] = {3, 15, 31, 63, 255, 1023, 65535, 655355};

    // generate random numbers to test codec
	srand(time(NULL));

	for (j = 0; j < 8; j++) {
		uint32_t last = test_max[0] - 1;
		uint32_t delta_max = test_max[j];

		printf("=== Unit test ===\n");
		printf("generating random ascending numbers:\n");

		input_arr[0] = 12345;
		printf("%u ", input_arr[0]);

		/* start testing ascending numbers with Max(delta) = delta_max */
		for (i = 0; i < len - 1; i++) {
			delta_arr[i] = last +
			               rand() / (RAND_MAX / (delta_max - last + 1) + 1);
			last = delta_arr[i];
			input_arr[i + 1] = input_arr[i] + delta_arr[i];

			printf("%u(delta=%u) ", input_arr[i + 1], delta_arr[i]);
		}
		printf("\n\n");

		/* acutal compress and decompress... */
		res = codec_compress(codec, input_arr, len, enc_arr);
		printf("compressing %u integers (%lu bytes) into %lu bytes "
		       "(b=%lu)...\n", len, len * sizeof(uint32_t), res,
		       ((struct for_delta_args*)(codec->args))->b);

		res = codec_decompress(codec, enc_arr, check_arr, len);
		printf("%lu bytes decompressed (b=%lu).\n",
		       res, ((struct for_delta_args*)(codec->args))->b);

		// check check_arr, see if it's same as input_arr.
		for (i = 0; i < len; i++)
			if(input_arr[i] != check_arr[i]) {
				printf("Somthing is wrong, please fix, %lu %u %u \n",
				       i ,input_arr[i], check_arr[i]);
				exit(1);
			}

		printf("pass unit test\n");
		printf("\n");
	}

    printf("\n*******pass all the tests*******\n");
	codec_free(codec);
	return 0;
}
