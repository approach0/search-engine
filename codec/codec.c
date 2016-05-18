#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "codec.h"

static size_t
for_delta_compress(const uint32_t *in, size_t len, void *out,
                   struct for_delta_args *args)
{
	return 0;
}

static size_t
dummpy_copy(const uint32_t *in, size_t len, void *out)
{
	size_t n_bytes = len << 2;
	memcpy(out, in, n_bytes);
	return n_bytes;
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
	} else if (codec->method == CODEC_PLAIN) {
		return dummpy_copy(in, len, out);
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
codec_decompress(struct codec *codec, const void *in,
                 uint32_t *out, size_t len)
{
	if (codec->method == CODEC_FOR_DELTA) {
		//...
	} else if (codec->method == CODEC_PLAIN) {
		return dummpy_copy(in, len, out);
	}

	return 0;
}

/*
 * Encode a structure (size `struct_sz') array `src' of length `n' into
 * a linear bytes stored by `dest_'. This function requires all structure
 * members be uint32_t integers. Each member of this structure is compressed
 * by corresponding codec specified in `codecs' array.
 *
 * Return the number of bytes of linear encoding.
 */
size_t
encode_struct_arr(void *dest_, const void *src, struct codec *codecs,
                  size_t n, size_t struct_sz)
{
	int i, j;
	uint32_t *p32src, *p32buf;
	uint32_t *intbuf;
	char     *dest = (char *)dest_;
	size_t   res, members = struct_sz >> 2 /* four bytes per interget */;

	intbuf = malloc(struct_sz * n);

	for (i = 0; i < members; i++) {
		p32buf = intbuf;
		p32src = (uint32_t*)src + i; /* adjust member offset */

		for (j = 0; j < n; j++) {
			*p32buf = *p32src;

			p32src += members;
			p32buf ++;
		}

		res = codec_compress(&codecs[i], intbuf, n, dest);
		dest += res;
	}

	free(intbuf);
	return (uintptr_t)dest - (uintptr_t)dest_;
}

/*
 * Reverse process of encode_struct_arr() function, parameters are
 * similar.
 *
 * Return the number of encoded bytes processed.
 */
size_t
decode_struct_arr(void *dest_, const void *src_, struct codec *codecs,
                  size_t n, size_t struct_sz)
{
	int i, j;
	uint32_t   *p32dest, *p32buf;
	uint32_t   *intbuf;
	const char *src = (const char *)src_;
	size_t      res, members = struct_sz >> 2 /* four bytes per interget */;

	intbuf = malloc(struct_sz * n);

	for (i = 0; i < members; i++) {
		res = codec_decompress(&codecs[i], src, intbuf, n);
		src += res;

		p32buf = intbuf;
		p32dest = (uint32_t*)dest_ + i; /* adjust member offset */

		for (j = 0; j < n; j++) {
			*p32dest = *p32buf;

			p32dest += members;
			p32buf ++;
		}
	}

	free(intbuf);
	return (uintptr_t)src - (uintptr_t)src_;
}
