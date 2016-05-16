#include "codec.h"

static size_t
for_delta_compress(const uint32_t *in, void *out,
                    size_t len, struct for_delta_args *args)
{
	return 0;
}

size_t
codec_compress(struct codec *codec, const uint32_t *in, void *out, size_t len)
{
	if (codec->method == CODEC_FOR_DELTA) {
		return for_delta_compress(in, out, len,
		                          (struct for_delta_args*)codec->args);
	}

	return 0;
}

size_t
codec_decompress(struct codec *codec, void *in, uint32_t *out, size_t len)
{
	if (codec->method == CODEC_FOR_DELTA) {
		//...
	}

	return 0;
}
