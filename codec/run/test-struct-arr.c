#include <string.h>
#include <stdio.h>
#include "searchd/math-search.h"
#include "codec.h"

#define LINEAR_ARRAY_LEN 1024
#define TEST_NUM 10
#define STRUCT_MEMBERS (sizeof(struct math_score_res) / sizeof(uint32_t))

int main()
{
	int i;
	size_t res;
	struct math_score_res test[TEST_NUM];
	uint32_t linear[LINEAR_ARRAY_LEN];
	struct for_delta_args for_args[2];
	struct codec codecs[] = {{CODEC_FOR_DELTA_ASC, for_args + 0},
	                         {CODEC_FOR_DELTA, for_args + 1},
	                         {CODEC_PLAIN, NULL}};

	printf("original structure array:\n");
	for (i = 0; i < TEST_NUM; i++) {
		test[i].doc_id = 123 + i;
		test[i].exp_id = 654 + i;
		test[i].score  = 998 + i;
		printf("(%u, %u, %u) ", test[i].doc_id,
		                        test[i].exp_id, test[i].score);
	}
	printf("\n");

	res = encode_struct_arr(linear, test, codecs, TEST_NUM,
	                        sizeof(struct math_score_res));

	printf("%lu bytes generated:\n", res);
	for (i = 0; i < STRUCT_MEMBERS * TEST_NUM; i++)
		printf("%u ", linear[i]);
	printf("\n");

	memset(test, 0, TEST_NUM * sizeof(struct math_score_res));
	res = decode_struct_arr(test, linear, codecs, TEST_NUM,
	                        sizeof(struct math_score_res));

	printf("restore %lu bytes back to structure array:\n", res);
	for (i = 0; i < TEST_NUM; i++) {
		printf("(%u, %u, %u) ", test[i].doc_id,
		                        test[i].exp_id, test[i].score);
	}
	printf("\n");

	return 0;
}
