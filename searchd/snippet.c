#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "list/list.h"
#include "snippet.h"

/* for MAX_TXT_SEG_BYTES */
#include "txt-seg/config.h"

/* for terminal colors */
#include "tex-parser/vt100-color.h"
#define SNIPPET_HL_COLOR C_RED
#define SNIPPET_HL_RST   C_RST

/* for SNIPPET_PADDING */
#include "config.h"

#define _min(x, y) ((x) > (y) ? (y) : (x))

struct snippet_div {
	/* position info */
	uint32_t kw_pos, kw_end;
	uint32_t pad_left, pad_right;
	bool     joint_left, joint_right;

	/* string buffers */
	char     left_str[SNIPPET_PADDING + 1];
	char     right_str[SNIPPET_PADDING + 1];
	char     kw_str[MAX_TXT_SEG_BYTES];

	struct list_node ln;
};

LIST_DEF_FREE_FUN(_free_div_list, struct snippet_div, ln, free(p));

void snippet_free_div_list(list* div_li)
{
	_free_div_list(div_li);
}

static LIST_CMP_CALLBK(compare_kw_pos)
{
	struct snippet_div
	*p0 = MEMBER_2_STRUCT(pa_node0, struct snippet_div, ln),
	*p1 = MEMBER_2_STRUCT(pa_node1, struct snippet_div, ln);

	return p0->kw_pos < p1->kw_pos;
}

void snippet_add_pos(list* div_li, char* kw_str,
                     uint32_t kw_pos, uint32_t kw_len)
{
	struct list_sort_arg sort = {&compare_kw_pos, NULL};
	struct snippet_div *s = malloc(sizeof(struct snippet_div));
	s->kw_pos = kw_pos;
	s->kw_end = kw_pos + kw_len;
	s->pad_left = s->pad_right = SNIPPET_PADDING;
	s->joint_left = s->joint_right = 0;

	s->left_str[0]  = '\0';
	s->right_str[0] = '\0';
	s->kw_str[0]    = '\0';

	LIST_NODE_CONS(s->ln);
	list_sort_insert(&s->ln, div_li, &sort);
}

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct snippet_div, div, ln);
	P_CAST(pos, bool, pa_extra);

	if (*pos) {
		printf("[%u]", div->pad_left);
		printf("{%u,%u}", div->kw_pos, div->kw_end);
		printf("[%u]", div->pad_right);
		printf(" ");
	} else {
		printf("%s", div->left_str);
		printf(SNIPPET_HL_COLOR "%s" SNIPPET_HL_RST,
		       div->kw_str);
		printf("%s", div->right_str);
	}

	if (!div->joint_right)
		printf(" ... ");

	LIST_GO_OVER;
}

void snippet_pos_print(list* div_li)
{
	bool pos = 1;
	list_foreach(div_li, &print, &pos);
	printf("\n");
}

void snippet_hi_print(list* div_li)
{
	bool pos = 0;
	list_foreach(div_li, &print, &pos);
	printf("\n");
}

static LIST_IT_CALLBK(align)
{
	uint32_t left, right, kw_dist;
	LIST_OBJ(struct snippet_div, div, ln);
	struct snippet_div* next_div = MEMBER_2_STRUCT(pa_fwd->now,
	                               struct snippet_div, ln);

	/* strip left padding if it exceeds buffer head */
	if (div->pad_left > div->kw_pos)
		div->pad_left = div->kw_pos;

	/* do the same thing for next_div (always not NULL) */
	if (next_div->pad_left > next_div->kw_pos)
		next_div->pad_left = next_div->kw_pos;

	if (pa_now->now != pa_head->last) {
		/* calculate the rightmost of this div and the
		 * leftmost of the next div */
		left = div->kw_end + div->pad_right;
		right = next_div->kw_pos - next_div->pad_left;

		/* align them if overlap */
		if (right < left) {
			kw_dist = next_div->kw_pos - div->kw_end;
			div->pad_right = _min(SNIPPET_PADDING, kw_dist);
			next_div->pad_left = next_div->kw_pos -
			                     (div->kw_end + div->pad_right);
			/* it follows that next_div->pad_left can only be
			 * either zero or (kw_dist - SNIPPET_PADDING). */

			div->joint_right = 1;
			next_div->joint_left = 1;
		}

		return LIST_RET_CONTINUE;
	} else {
		return LIST_RET_BREAK;
	}
}

/* test if a char is an UTF-8 continuation byte */
static bool utf8_conti(char byte)
{
	if ((byte & 0x80) &&
	    !(byte & 0x40))
		return 1; /* starting with binary 10 */
	else
		return 0;
}

static uint32_t
strip_utf8conti_from_head(char *buf, size_t len)
{
	size_t i, j;
	/* go to the first byte that is utf8 start byte */
	for (i = 0; i < len; i++)
		if (!utf8_conti(buf[i]))
			break;

	if (i != len) {
		/* move right utf8 string ahead and overwrite the head */
		for (j = 0; i < len; i++, j++) {
			buf[j] = buf[i];
		}

		buf[j] = '\0';
		return (uint32_t)j;
	}

	return 0;
}

static uint32_t
strip_utf8conti_from_tail(char *buf, size_t len)
{
	size_t i = len;
	/* go reversely from the tail, to the first byte that
	 * is utf8 start byte */
	do {
		i --;
		if (!utf8_conti(buf[i]))
			break;
	} while (i != 0);

	/* cut it and what follows */
	buf[i] = '\0';
	return (uint32_t)i;
}

static LIST_IT_CALLBK(read_file)
{
	size_t nread;
	uint32_t newlen;
	LIST_OBJ(struct snippet_div, div, ln);
	P_CAST(fh, FILE, pa_extra);

	/* re-seek file from beginning if left div is not joint */
	if (!div->joint_left)
		fseek(fh, div->kw_pos - div->pad_left, SEEK_SET);

	/* read into left_str buffer */
	nread = fread(div->left_str, 1, div->pad_left, fh);
	div->left_str[nread] = '\0';
	if (!div->joint_left) {
		newlen = strip_utf8conti_from_head(div->left_str, nread);
		div->pad_left = newlen;
	}

	/* read into keyword buffer */
	nread = fread(div->kw_str, 1, div->kw_end - div->kw_pos, fh);
	div->kw_str[nread] = '\0';

	/* read into right_str buffer */
	nread = fread(div->right_str, 1, div->pad_right, fh);
	div->right_str[nread] = '\0';
	if (!div->joint_right) {
		newlen = strip_utf8conti_from_tail(div->right_str, nread);
		div->pad_right = newlen;
	}

	LIST_GO_OVER;
}

static void _rm_linefeed(char *str, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		if (str[i] == '\n')
			str[i] = '|';
		else if (str[i] == '\r')
			str[i] = '|';
	}
}

static LIST_IT_CALLBK(rm_linefeed)
{
	LIST_OBJ(struct snippet_div, div, ln);
	_rm_linefeed(div->left_str, div->pad_left);
	_rm_linefeed(div->right_str, div->pad_right);

	LIST_GO_OVER;
}

void snippet_read_file(FILE* fh, list* div_li)
{
	list_foreach(div_li, &align, NULL);
	list_foreach(div_li, &read_file, fh);
	list_foreach(div_li, &rm_linefeed, NULL);
}

void snippet_read_blob(void* blob, size_t blob_sz, list* div_li)
{
	/* still nothing */
}

char *snippet_highlight(list* div_li, char *open, char *close)
{
	return malloc(1);
}
