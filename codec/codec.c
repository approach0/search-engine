#include "codec.h"
#include <stdio.h>

static size_t
for_delta_compress(const uint32_t *in, size_t len, int *out,
                   struct for_delta_args *args)
{
    unsigned bsets[8] ={2, 4, 5, 6, 8, 10, 16, 32};
	// find the maximum element in the input array
    // must declare MAX as long integer because it would
    // cause bit shift operation (MAX>>bsets[bn]) overflow when b is 32
    long int MAX = in[0];
    for(int i = 1; i < len; ++i)
        if(in[i] > MAX)
            MAX = in[i];
    // find the b value for FOR_DELTA algorithm
    unsigned bn = 0;
    unsigned b;
    while((MAX>>bsets[bn])>0) bn++;
    b = bsets[bn];
    printf("b is %u \n", b);
    args->b = b;
    // compress each element of input array using b bits
    unsigned out_size=(len+32/b-1)/(32/b);
    unsigned total_bytes = out_size*4;
    args->pack_size = total_bytes;
    unsigned threshold;
    // the maximum width of shift operation is 31
    if(b == 32)
        threshold = 1<<31;
    else
        threshold = 1<<b;
    for(int i = 0; i < len; ++i){
        unsigned low = in[i] & (threshold-1);
        out[i/(32/b)] |= low << i%(32/b)*b;
    }
    return total_bytes;
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
               int *out)
{
	if (codec->method == CODEC_FOR_DELTA) {
		return for_delta_compress(in, len, out,
		                          (struct for_delta_args*)codec->args);
	}

	return 0;
}

void step2b(int num, int* in, int* out){
    unsigned l=(num+15)/16;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 3;
       out[1] = (block>>2) & 3;
       out[2] = (block>>4) & 3;
       out[3] = (block>>6) & 3;
       out[4] = (block>>8) & 3;
       out[5] = (block>>10) & 3;
       out[6] = (block>>12) & 3;
       out[7] = (block>>14) & 3;
       out[8] = (block>>16) & 3;
       out[9] = (block>>18) & 3;
       out[10] = (block>>20) & 3;
       out[11] = (block>>22) & 3;
       out[12] = (block>>24) & 3;
       out[13] = (block>>26) & 3;
       out[14] = (block>>28) & 3;
       out[15] = (block>>30) & 3;
       out+=16;
    }
}

void step4b(int num, int* in, int* out){
    unsigned l=(num+7)/8;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 15;
       out[1] = (block>>4) & 15;
       out[2] = (block>>8) & 15;
       out[3] = (block>>12) & 15;
       out[4] = (block>>16) & 15;
       out[5] = (block>>20) & 15;
       out[6] = (block>>24) & 15;
       out[7] = (block>>28) & 15;
       out+=8;
    }
}

void step5b(int num, int* in, int* out){
    unsigned l=(num+5)/6;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 31;
       out[1] = (block>>5) & 31;
       out[2] = (block>>10) & 31;
       out[3] = (block>>15) & 31;
       out[4] = (block>>20) & 31;
       out[5] = (block>>25) & 31;
       out+=6;
    }
}

void step6b(int num, int* in, int* out){
    unsigned l=(num+4)/5;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 63;
       out[1] = (block>>6) & 63;
       out[2] = (block>>12) & 63;
       out[3] = (block>>18) & 63;
       out[4] = (block>>24) & 63;
       out+=5;
    }
}

void step8b(int num, int* in, int* out){
    unsigned l=(num+3)/4;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 255;
       out[1] = (block>>8) & 255;
       out[2] = (block>>16) & 255;
       out[3] = (block>>24) & 255;
       out+=4;
    }
}

void step10b(int num, int* in, int* out){
    unsigned l=(num+2)/3;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 1023;
       out[1] = (block>>10) & 1023;
       out[2] = (block>>20) & 1023;
       out+=3;
    }
}

void step16b(int num, int* in, int* out){
    unsigned l=(num+1)/2;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & ((1<<16)-1);
       out[1] = block>>16;
       out+=2;
    }
}

void step32b(int num, int* in, int* out){
    unsigned l=num;
    unsigned i,block;
    for(i=0;i<l;i++)
    {
       block=in[i];
       out[0] = block & 0xFFFFFFFF;
       out+=1;
    }
}

void (*steps[33])(int num, int* in, int* out);
/*
 * Decompress the compressed uint32_t integer array `in' whose original
 * length is `len', using the algorithm and its parameters specified from
 * `codec'. It writes the decompressed content to `out'.
 *
 * Return the number of compressed bytes processed (return 0 on error).
 */
size_t
codec_decompress(struct codec *codec, int *in, uint32_t *out, size_t len)
{
	if (codec->method == CODEC_FOR_DELTA) {
        // the name of a function and a pointer to
        // the same function are interchangeable
        steps[2] = step2b;
        steps[4] = step4b;
        steps[5] = step5b;
        steps[6] = step6b;
        steps[8] = step8b;
        steps[10] = step10b;
        steps[16] = step16b;
        steps[32] = step32b;

        unsigned b = ((struct for_delta_args*)(codec->args))->b;
        (*steps[b])(len, in, out);
	}

	return 0;
}
