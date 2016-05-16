#include <stdio.h>
#include "codec.h"

int main()
{
	const int len = 10;
	int i;
	struct codec codec;
	struct for_delta_args args;
	uint32_t res, input_arr[len], check_arr[len]; 
	char buf[1024];

	codec.method = CODEC_FOR_DELTA;
	codec.args = &args;

	for (i = 0; i < len; i++)
		input_arr[i] = i;

	res = codec_compress(&codec, input_arr, buf, len);

	printf("compressing %u integers into %u bytes...\n",
	       len, res);

	codec_decompress(&codec, buf, check_arr, len);

	// check check_arr, see if it's same as input_arr.
	
	return 0;
}
