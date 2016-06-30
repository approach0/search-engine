#include <stdio.h>
#include "codec.h"

int main()
{
	const int len = 10;
	int i;
	struct codec codec = {CODEC_PLAIN, NULL};
	uint32_t res, input_arr[len], check_arr[len];
	char buf[1024];

	for (i = 0; i < len; i++)
		input_arr[i] = i;

	res = codec_compress(&codec, input_arr, len, buf);

	printf("compressing %u integers into %u bytes...\n",
	       len, res);

	codec_decompress(&codec, buf, check_arr, len);

	// check check_arr, see if it's same as input_arr.
	for (i = 0; i < len; i++)
		printf("%u ", check_arr[i]);
	printf("\n");

	return 0;
}
