#include <stdio.h>
#include <string.h>
#include "codec.h"

int main()
{
	struct codec codec = {CODEC_GZ, NULL};
	size_t res;
	char text[] = "The initialization of the state is the same, except that there is no compression level, of course, and two more elements of the structure are initialized. avail_in and next_in must be initialized before calling inflateInit(). This is because the application has the option to provide the start of the zlib stream in order for inflateInit() to have access to information about the compression method to aid in memory allocation. In the current implementation of zlib (up through versions 1.2.x), the method-dependent memory allocations are deferred to the first call of inflate() anyway. However those fields must be initialized since later versions of zlib that provide more compression methods may take advantage of this interface. In any case, no decompression is performed by inflateInit(), so the avail_out and next_out fields do not need to be initialized before calling.";
	long unsigned int text_len = strlen(text) + 1;
	void *compressed;

	const int check_str_len = 4096;
	char check_str[check_str_len];
	printf("originally %lu bytes.\n", text_len);
	printf("\n");

	/* compression */
	res = codec_compress(&codec, text, text_len, &compressed);
	printf("compressed into %lu bytes.\n", res);
	printf("\n");

	/* decompression */
	res = codec_decompress(&codec, compressed, res, check_str, check_str_len);
	printf("uncompressed back to %lu bytes.\n", res);
	printf("check string:\n");
	printf("%s\n", check_str);
	printf("\n");

	/* clear */
	memset(check_str, 0, check_str_len);

	/* demo a small-size destination buffer failure */
	res = codec_decompress(&codec, compressed, res,
	                               check_str, 3 /* too small size */);
	printf("uncompressed back to %lu bytes.\n", res);
	printf("check string:\n");
	printf("%s\n", check_str);

	/* free compressed buffer */
	free(compressed);
	return 0;
}
