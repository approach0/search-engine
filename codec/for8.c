#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "for8.h"

size_t
for8_compress(uint8_t *in, size_t len, uint8_t *out, size_t *b_)
{
	int i;
	uint64_t max;

	size_t b /* bits per compressed value */,
	       n_val /* compressed values per 32 bits */,
	       out_sz /* compressed stream size */;

	size_t b_set[3] /* b candidates */
		= {2, 4, 8};

	/* output buffer header */
	uint8_t *head = (uint8_t*)out;

	/* safe guard */
	if (len == 0)
		return 0;

	/* find the maximum element from input array */
	max = in[0];
	for(i = 1; i < len; ++i)
		if(in[i] > max)
			max = in[i];

	/* find the b value enough to hold the max value */
	i = 0;
	while ((max >> b_set[i]) > 0) i++;
	b = b_set[i];

	/* write b to output buffer header */
	*head = b;
	out = (uint8_t*)(head + 1);

	/* calculate some values */
	n_val = 8 / b;
	out_sz = ((len - 1) / n_val + 1) /* i.e. round up (len / n_val) */;
	out_sz = out_sz << 0; /* convert to bytes */
	//printf("using %u, len %u, out_sz %u\n", b, len, out_sz);

	/* first initialize output array to all zeros, since
	 * elements in output array will logical OR with
	 * input data. */
	memset(out, 0, out_sz);

	/* finally, frame of reference encode */
	for(i = 0; i < len; ++i)
		out[i / n_val] |= in[i] << (i % n_val) * b;

	*b_ = b;
	return sizeof(uint8_t) + out_sz;
}

static uint32_t for_decompress_b2(size_t len, uint8_t* in, uint8_t* out)
{
	uint32_t i, l = (len + 4 - 1) / 4;
	uint8_t block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0]  = block         & 3;
		out[1]  = (block >> 2)  & 3;
		out[2]  = (block >> 4)  & 3;
		out[3]  = (block >> 6)  & 3;
		out += 4;
	}

	return l;
}

static uint32_t for_decompress_b4(size_t len, uint8_t* in, uint8_t* out)
{
	uint32_t i, l = (len + 2 - 1) / 2;
	uint8_t block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 15;
		out[1] = (block >> 4)  & 15;
		out += 2;
	}

	return l;
}

static uint32_t for_decompress_b8(size_t len, uint8_t* in, uint8_t* out)
{
	uint32_t i, l = len;
	uint8_t block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 255;
		out += 1;
	}

	return l;
}

size_t for8_decompress(uint8_t* in, uint8_t* out, size_t len, size_t *b_)
{
	size_t   l;
	uint8_t *head = (uint8_t*)in; /* points to b info */

	uint8_t tmp[len + 3];

#define B_CASE(_b) \
	case _b: \
		l = for_decompress_b ## _b(len, head + 1, tmp); \
		break

	switch (*head /* b */) {
		B_CASE(2);
		B_CASE(4);
		B_CASE(8);

	default:
		assert(0);
	}

	/* copy tmp content */
	memcpy(out, tmp, len);

	*b_ = *head;
	return sizeof(uint8_t) + l;
}
