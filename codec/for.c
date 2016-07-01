#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "for.h"

size_t
for_compress(uint32_t *in, size_t len, uint32_t *out, size_t *b_)
{
	int i;
	uint64_t max; /* made 64 bits long so that expression
	(max >> 32) will not cause undefined behavior */

	size_t b /* bits per compressed value */,
	       n_val /* compressed values per 32 bits */,
	       out_sz /* compressed stream size */;

	size_t b_set[8] /* b candidates */
		= {2, 4, 5, 6, 8, 10, 16, 32};
	/* b candidates are selected such that (32 bits / b) will
	 * result in as many as different integers.  */

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
	out = (uint32_t*)(head + 1);

	/* calculate some values */
	n_val = 32 / b;
	out_sz = ((len - 1) / n_val + 1) /* i.e. round up (len / n_val) */;
	out_sz = out_sz << 2; /* convert to bytes */

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

static uint32_t for_decompress_b2(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 16 - 1) / 16;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0]  = block         & 3;
		out[1]  = (block >> 2)  & 3;
		out[2]  = (block >> 4)  & 3;
		out[3]  = (block >> 6)  & 3;
		out[4]  = (block >> 8)  & 3;
		out[5]  = (block >> 10) & 3;
		out[6]  = (block >> 12) & 3;
		out[7]  = (block >> 14) & 3;
		out[8]  = (block >> 16) & 3;
		out[9]  = (block >> 18) & 3;
		out[10] = (block >> 20) & 3;
		out[11] = (block >> 22) & 3;
		out[12] = (block >> 24) & 3;
		out[13] = (block >> 26) & 3;
		out[14] = (block >> 28) & 3;
		out[15] = (block >> 30) & 3;
		out += 16;
	}

	return l;
}

static uint32_t for_decompress_b4(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 8 - 1) / 8;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 15;
		out[1] = (block >> 4)  & 15;
		out[2] = (block >> 8)  & 15;
		out[3] = (block >> 12) & 15;
		out[4] = (block >> 16) & 15;
		out[5] = (block >> 20) & 15;
		out[6] = (block >> 24) & 15;
		out[7] = (block >> 28) & 15;
		out += 8;
	}

	return l;
}

static uint32_t for_decompress_b5(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 6 - 1) / 6;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 31;
		out[1] = (block >> 5)  & 31;
		out[2] = (block >> 10) & 31;
		out[3] = (block >> 15) & 31;
		out[4] = (block >> 20) & 31;
		out[5] = (block >> 25) & 31;
		out += 6;
	}

	return l;
}

static uint32_t for_decompress_b6(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 5 - 1) / 5;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 63;
		out[1] = (block >> 6)  & 63;
		out[2] = (block >> 12) & 63;
		out[3] = (block >> 18) & 63;
		out[4] = (block >> 24) & 63;
		out += 5;
	}

	return l;
}

static uint32_t for_decompress_b8(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 4 - 1) / 4;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 255;
		out[1] = (block >> 8)  & 255;
		out[2] = (block >> 16) & 255;
		out[3] = (block >> 24) & 255;
		out += 4;
	}

	return l;
}

static uint32_t for_decompress_b10(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 3 - 1) / 3;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block)       & 1023;
		out[1] = (block >> 10) & 1023;
		out[2] = (block >> 20) & 1023;
		out += 3;
	}

	return l;
}

static uint32_t for_decompress_b16(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = (len + 2 - 1) / 2;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = (block) & ((1 << 16) - 1); /* lower 16 bits mask */
		out[1] = block >> 16;
		out += 2;
	}

	return l;
}

static uint32_t for_decompress_b32(size_t len, uint32_t* in, uint32_t* out)
{
	uint32_t l = len;
	uint32_t i, block;

	for (i = 0; i < l; i++) {
		block = in[i];
		out[0] = block;
		out += 1;
	}

	return l;
}

size_t for_decompress(uint32_t* in, uint32_t* out, size_t len, size_t *b_)
{
	size_t   l;
	uint8_t *head = (uint8_t*)in; /* points to b info */

#define B_CASE(_b) \
	case _b: \
		l = for_decompress_b ## _b(len, (uint32_t*)(head + 1), out); \
		break

	switch (*head /* b */) {
		B_CASE(2);
		B_CASE(4);
		B_CASE(5);
		B_CASE(6);
		B_CASE(8);
		B_CASE(10);
		B_CASE(16);
		B_CASE(32);

	default:
		assert(0);
	}

	*b_ = *head;
	return sizeof(uint8_t) + (l << 2);
}

size_t
for_delta_compress(const uint32_t *in, size_t len, uint32_t *out, size_t *b_)
{
	size_t for_sz, b, i;

	uint32_t *delta_buf;
	uint32_t *head = out;

	/* safe guard */
	if (len == 0)
		return 0;
	else
		delta_buf = malloc((len - 1) * sizeof(uint32_t));

	/* len is greater than zero, write the first number (initial value) */
	*head = in[0];

	if (len == 1) {
		for_sz = 0;
		b = 0;

	} else /* `len' is greater than one */ {

		/* convert `in' buffer to `delta_buf' */
		for (i = 1; i < len; i++)
			delta_buf[i - 1] = in[i] - in[i - 1];

		/* following the init value is our delta encodes */
		for_sz = for_compress(delta_buf, len - 1, head + 1, &b);
	}

	*b_ = b;
	free(delta_buf);
	return sizeof(uint32_t) + for_sz;
}

size_t
for_delta_decompress(const uint32_t *in, uint32_t *out, size_t len, size_t *b_)
{
	size_t for_sz, b, i;

	uint32_t *delta_buf;
	uint32_t *head = (uint32_t*)in;

	/* safe guard */
	if (len == 0)
		return 0;
	else
		delta_buf = malloc((len - 1 + 15) * sizeof(uint32_t));
		/* for_decompress() function in some cases, may output
		 * more than `len' 32bits integers. In worst case, where
		 * only one 32bits integer is encoded (len=1) with b=2,
		 * according to for_decompress_b2() function, in this case,
		 * decoding will write output buffer to the furthest at
		 * `out[15]'. Therefore sufficient extra space is necessary.
		 * (single 32bits can generate 16 out[], thus we need
		 * 16 - 1 = 15 extra 32bits space in worst case)
		 */

	/* len is greater than zero, get and write initial value to output */
	out[0] = *head;

	if (len == 1) {
		b = 0;
		for_sz = 0;

	} else /* will be more than one values in decoded buffer */ {

		/* following the init value is our delta encodes */
		for_sz = for_decompress(head + 1, delta_buf, len - 1, &b);

		/* convert `delta_buf' to `out' buffer */
		for (i = 1; i < len; i++)
			out[i] = delta_buf[i - 1] + out[i - 1];
	}

	*b_ = b;
	free(delta_buf);
	return sizeof(uint32_t) + for_sz;
}
