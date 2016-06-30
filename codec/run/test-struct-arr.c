#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "searchd/math-search.h"
#include "codec.h"

#define LINEAR_ARRAY_LEN 1024
#define TEST_NUM 30
#define STRUCT_MEMBERS (sizeof(struct math_score_res) / sizeof(uint32_t))

int main()
{
	int i;
	size_t res;
	struct math_score_res test[TEST_NUM];
	uint32_t linear[LINEAR_ARRAY_LEN];

	struct codec *codecs[] = {
/* first member codec mathod: */
#if 0
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
#else
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
#endif

/* second member codec mathod: */
#if 0
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
#else
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
#endif

/* third member codec mathod: */
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	};

	srand(time(NULL));
	printf("original structure array (%lu bytes):\n",
	       STRUCT_MEMBERS * TEST_NUM * sizeof(uint32_t));
	for (i = 0; i < TEST_NUM; i++) {
		test[i].doc_id = 123 + i;
		test[i].exp_id = i % 5;
		test[i].score  = rand() % 100;
		printf("(%u, %u, %u) ", test[i].doc_id,
		                        test[i].exp_id, test[i].score);
	}
	printf("\n");

	res = encode_struct_arr(linear, test, codecs, TEST_NUM,
	                        sizeof(struct math_score_res));
	printf("encoding... (b[0] = %lu, b[1] = %lu)\n",
	       ((struct for_delta_args*)codecs[0]->args)->b,
	       ((struct for_delta_args*)codecs[1]->args)->b);

	printf("{{{ %lu }}} bytes generated:\n", res);
	for (i = 0; i < STRUCT_MEMBERS * TEST_NUM; i++)
		printf("%u ", linear[i]);
	printf("\n");

	memset(test, 0, TEST_NUM * sizeof(struct math_score_res));
	res = decode_struct_arr(test, linear, codecs, TEST_NUM,
	                        sizeof(struct math_score_res));
	printf("decoding... (b[0] = %lu, b[1] = %lu)\n",
	       ((struct for_delta_args*)codecs[0]->args)->b,
	       ((struct for_delta_args*)codecs[1]->args)->b);

	printf("restore %lu bytes back to structure array:\n", res);
	for (i = 0; i < TEST_NUM; i++) {
		printf("(%u, %u, %u) ", test[i].doc_id,
		                        test[i].exp_id, test[i].score);
	}
	printf("\n");

	for (i = 0; i < sizeof(codecs) / sizeof(struct codec*); i++)
		codec_free(codecs[i]);

	return 0;
}
