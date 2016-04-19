#include <stdio.h>
#include <stdlib.h>
#include "txt-seg.h"

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct term_list_node, p, ln);

	printf("%S", p->term);
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
	"人艰不拆"
};

#define n_txt (sizeof(txt) / sizeof(const char *))

int main()
{
	list li = LIST_NULL;
	text_segment_init("../jieba/fork/dict");
	int i;

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
