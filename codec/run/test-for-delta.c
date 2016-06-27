#include <stdio.h>
#include "codec.h"
#include <stdlib.h>
#include <time.h>
#include <string.h> /* for memset() */
int main()
{
	const int len = 1024;
	int i;
	struct codec codec;
	struct for_delta_args args;
	uint32_t res;
    uint32_t* input_arr;
    input_arr  = (uint32_t*)malloc(sizeof(uint32_t)*len);
    uint32_t* check_arr;
    check_arr  = (uint32_t*)malloc(sizeof(uint32_t)*len);
    int* buf = (int*)malloc(sizeof(int)*1024);

    // define a array to test each case for different b
    int N_Max[8] = {3, 15, 31, 63, 255, 1023, 65535, 655355};
    //int N_Max[1] = {15};
	codec.method = CODEC_FOR_DELTA;
	codec.args = &args;
    // generate random numbers to test codec
        srand(time(NULL));
    for(int j=0; j<8; ++j){
        int M = N_Max[0] - 1; // minimum number
        int N = N_Max[j]; // maximum number
        for (i = 0; i < len; i++){
			int last;
			if (i == 0)
				last = M;
			else
				last = input_arr[i - 1];

            input_arr[i] = last + rand() / (RAND_MAX / (N - last + 1) + 1);
			//printf("%u ", input_arr[i]);
        }
		//printf("\n");
        res = codec_compress(&codec, input_arr, len, buf);
		printf("b is %u \n", args.b);
        printf("compressing %u integers into %u bytes...\n",
               len, res);
        res = codec_decompress(&codec, buf, check_arr, len);
		printf("%u bytes decompressed.\n", res);
        // check check_arr, see if it's same as input_arr.
        for (i = 0; i < len; i++)
            if(input_arr[i] != check_arr[i]){
                printf("Somthing is wrong, please fix, %d %d  %d \n", i ,input_arr[i], check_arr[i]);
                exit(1);
            }
        printf("pass unit test\n");
        // set buf to zero again
        memset(buf, 0, sizeof(int)*1024);
    }
    printf("\n*******pass all the tests*******\n");
    free(check_arr);
    free(input_arr);
    free(buf);
	return 0;
}
