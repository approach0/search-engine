#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "list/list.h"
#include "snippet.h"

/* for MAX_TXT_SEG_BYTES */
#include "txt-seg/config.h"

/* even a short query can get a long hilighted string, e.g.
 * in the case of math search */
#define MAX_HIGHLIGHTED_BYTES (MAX_TXT_SEG_BYTES * 1024)

/* for terminal colors */
#include "tex-parser/vt100-color.h"
#define SNIPPET_HL_COLOR C_RED
#define SNIPPET_HL_RST   C_RST

/* for SNIPPET_PADDING and MAX_SNIPPET_SZ */
#include "config.h"

#define _min(x, y) ((x) > (y) ? (y) : (x))

struct snippet_hi {
	/* position info */
	uint32_t kw_pos, kw_end;
	uint32_t pad_left, pad_right;
	bool     joint_left, joint_right;

	/* string buffers */
	char     left_str[SNIPPET_PADDING + 1];
	char     right_str[SNIPPET_PADDING + 1];
	char     kw_str[MAX_HIGHLIGHTED_BYTES];

	struct list_node ln;
};

struct write_snippet_arg {
	char *snippet;
	const char *open, *close;
};

LIST_DEF_FREE_FUN(free_hi_list, struct snippet_hi, ln, free(p));

void snippet_free_highlight_list(list* hi_li)
{
	free_hi_list(hi_li);
}

static LIST_CMP_CALLBK(compare_kw_pos)
{
	struct snippet_hi
	*p0 = MEMBER_2_STRUCT(pa_node0, struct snippet_hi, ln),
	*p1 = MEMBER_2_STRUCT(pa_node1, struct snippet_hi, ln);

	return p0->kw_pos < p1->kw_pos;
}

void snippet_push_highlight(list* hi_li, char* kw_str,
                            uint32_t kw_pos, uint32_t kw_len)
{
	struct snippet_hi *h = malloc(sizeof(struct snippet_hi));
	h->kw_pos = kw_pos;
	h->kw_end = kw_pos + kw_len;
	h->pad_left = h->pad_right = SNIPPET_PADDING;
	h->joint_left = h->joint_right = 0;

	h->left_str[0]  = '\0';
	h->right_str[0] = '\0';
	h->kw_str[0]    = '\0';

	LIST_NODE_CONS(h->ln);
	list_insert_one_at_tail(&h->ln, hi_li, NULL, NULL);
}

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct snippet_hi, h, ln);
	P_CAST(pos, bool, pa_extra);

	if (*pos) {
		printf("[%u]", h->pad_left);
		printf("{%u,%u}", h->kw_pos, h->kw_end);
		printf("[%u]", h->pad_right);
		printf(" ");
	} else {
		printf("%s", h->left_str);
		printf(SNIPPET_HL_COLOR "%s" SNIPPET_HL_RST,
		       h->kw_str);
		printf("%s", h->right_str);
	}

	if (!h->joint_right)
		printf(" ... ");

	LIST_GO_OVER;
}

void snippet_pos_print(list* hi_li)
{
	bool if_print_pos = 1;
	list_foreach(hi_li, &print, &if_print_pos);
	printf("\n");
}

void snippet_hi_print(list* hi_li)
{
	bool if_print_pos = 0;
	list_foreach(hi_li, &print, &if_print_pos);
	printf("\n");
}

static LIST_IT_CALLBK(align)
{
	uint32_t left, right, kw_dist;
	LIST_OBJ(struct snippet_hi, h, ln);
	struct snippet_hi* next_h =
		MEMBER_2_STRUCT(pa_fwd->now, struct snippet_hi, ln);

	/* strip left padding if it exceeds buffer head */
	if (h->pad_left > h->kw_pos)
		h->pad_left = h->kw_pos;

	/* do the same thing for next_h (always not NULL) */
	if (next_h->pad_left > next_h->kw_pos)
		next_h->pad_left = next_h->kw_pos;

	if (pa_now->now != pa_head->last) {
		/* calculate the rightmost of this h and the
		 * leftmost of the next h */
		left = h->kw_end + h->pad_right;
		right = next_h->kw_pos - next_h->pad_left;

		/* align them if overlap */
		if (right < left) {
			kw_dist = next_h->kw_pos - h->kw_end;
			h->pad_right = _min(SNIPPET_PADDING, kw_dist);
			next_h->pad_left = next_h->kw_pos - (h->kw_end + h->pad_right);
			/* it follows that next_h->pad_left can only be
			 * either zero or (kw_dist - SNIPPET_PADDING). */

			h->joint_right = 1;
			next_h->joint_left = 1;
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
	LIST_OBJ(struct snippet_hi, h, ln);
	P_CAST(fh, FILE, pa_extra);

	/* re-seek file from beginning if left h is not joint */
	if (!h->joint_left)
		fseek(fh, h->kw_pos - h->pad_left, SEEK_SET);

	/* read into left_str buffer */
	nread = fread(h->left_str, 1, h->pad_left, fh);
	h->left_str[nread] = '\0';
	if (!h->joint_left) {
		newlen = strip_utf8conti_from_head(h->left_str, nread);
		h->pad_left = newlen;
	}

	/* read into keyword buffer */
	nread = fread(h->kw_str, 1, h->kw_end - h->kw_pos, fh);
	h->kw_str[nread] = '\0';

	/* read into right_str buffer */
	nread = fread(h->right_str, 1, h->pad_right, fh);
	h->right_str[nread] = '\0';
	if (!h->joint_right) {
		newlen = strip_utf8conti_from_tail(h->right_str, nread);
		h->pad_right = newlen;
	}

	LIST_GO_OVER;
}

static void _rm_linefeed(char *str, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		if (str[i] == '\n')
			str[i] = ' ';
		else if (str[i] == '\r')
			str[i] = ' ';
	}
}

static LIST_IT_CALLBK(rm_linefeed)
{
	LIST_OBJ(struct snippet_hi, h, ln);
	_rm_linefeed(h->left_str, h->pad_left);
	_rm_linefeed(h->right_str, h->pad_right);

	LIST_GO_OVER;
}

void snippet_read_file(FILE* fh, list* hi_li)
{
#ifdef DEBUG_SNIPPET
	printf("snippet right before reading file:\n");
	snippet_pos_print(hi_li);
#endif

	list_foreach(hi_li, &align, NULL);
	list_foreach(hi_li, &read_file, fh);
	list_foreach(hi_li, &rm_linefeed, NULL);
}

static LIST_IT_CALLBK(write_snippet)
{
	LIST_OBJ(struct snippet_hi, h, ln);
	P_CAST(arg, struct write_snippet_arg, pa_extra);
	char *p = arg->snippet;

	p += sprintf(p, "%s", h->left_str);
	p += sprintf(p, "%s", arg->open);
	p += sprintf(p, "%s", h->kw_str);
	p += sprintf(p, "%s", arg->close);
	p += sprintf(p, "%s", h->right_str);

	if (!h->joint_right)
		p += sprintf(p, " ... ");

	LIST_GO_OVER;
}

const char
*snippet_highlighted(list* hi_li, const char *open, const char *close)
{
	static char snippet[MAX_SNIPPET_SZ];
	struct write_snippet_arg arg = {snippet, open, close};
	list_foreach(hi_li, &write_snippet, &arg);

	return snippet;
}
