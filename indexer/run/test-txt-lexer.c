#include "txt-seg.h"
extern int txtlex();

int main()
{
	printf("opening dict...\n");
	text_segment_init("../jieba/clone/dict");
	printf("dict opened.\n");

	txtlex();

	printf("closing dict...\n");
	text_segment_free();
}
