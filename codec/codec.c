#include "codec.h"

static size_t
for_delta_compress(const uint32_t *in, size_t len, void *out,
                   struct for_delta_args *args)
{
	return 0;
}

/*
 * Compress an uint32_t integer array `in' of length `len', using the
 * algorithm and its parameters specified from `codec'. It writes the
 * compressed content to buffer `out'.
 *
 * Return the number of bytes of compressed buffer (return 0 on error).
 */
size_t
codec_compress(struct codec *codec,
               const uint32_t *in, size_t len,
               void *out)
{
	if (codec->method == CODEC_FOR_DELTA) {
		return for_delta_compress(in, len, out,
		                          (struct for_delta_args*)codec->args);
	}

	return 0;
}

/*
 * Decompress the compressed uint32_t integer array `in' whose original
 * length is `len', using the algorithm and its parameters specified from
 * `codec'. It writes the decompressed content to `out'.
 *
 * Return the number of compressed bytes processed (return 0 on error).
 */
size_t
codec_decompress(struct codec *codec, void *in, uint32_t *out, size_t len)
{
	if (codec->method == CODEC_FOR_DELTA) {
		//...
	}

	return 0;
}
