#include <stdio.h>
#include <stdlib.h>
#include "txt-seg.h"

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct term_list_node, p, ln);

	printf("%S<%u,%u>'%c'", p->term, p->begin_pos, p->end_pos,
	                        (p->type == TERM_T_ENGLISH) ? 'e' : 'c');
	if (pa_now->now == pa_head->last)
		printf(".");
	else
		printf(", ");

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node,
                  ln, free(p));

const char * txt[] = {
	"General AND/OR and score 阶段: 使用 vector space model 进行 query / doc 的整体评分（如果大于 heap min element 的话）。",
	"非负整数集",
	"一元二次不等式",
	"最小正周期",
	"洛必达法则",
	"几何平均数",
	"南京市长江大桥",
	"C++和c#是什么关系？11+122=133，是吗？PI=3.14159",
	"工信处女干事每月经过下属科室都要亲口交代24口交换机等技术性器件的安装工作",
	"“Microsoft”一词由“MICROcomputer（微型计算机）”和“SOFTware（软件）”两部分组成",
	"人艰不拆"
};

#define n_txt (sizeof(txt) / sizeof(const char *))

int main(void)
{
	list li = LIST_NULL;
	text_segment_init("../jieba/fork/dict");
	unsigned int i;

	text_segment_insert_usrterm("人艰不拆");

	for (i = 0; i < n_txt; i++) {
		li = text_segment(txt[i]);
		list_foreach(&li, &print, NULL);
		printf("\n");
		list_release(&li);
	}

	text_segment_free();
	return 0;
}
