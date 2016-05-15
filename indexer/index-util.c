#undef NDEBUG /* make sure assert() works */
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wstring/wstring.h"
#include "txt-seg/txt-seg.h"

#include "lex-slice.h"
#include "index-util.h"

struct split_arg {
	list             *into_list;
	struct lex_slice *slice;
	size_t            slice_chars;
};

static LIST_IT_CALLBK(conv_pos)
{
	LIST_OBJ(struct term_list_node, t, ln);
	P_CAST(sa, struct split_arg, pa_extra);
	struct splited_term *new_split;

	size_t term_bytes  = mbstr_bytes(t->term);
	uint32_t term_doc_offset = sa->slice->begin +
	                           term_bytes_offset(t->begin_pos);

	assert(t->end_pos <= sa->slice_chars);
	assert(term_bytes <= sa->slice->offset);

	new_split = malloc(sizeof(struct splited_term));
	strcpy(new_split->term, wstr2mbstr(t->term));
	new_split->doc_pos = term_doc_offset;
	new_split->n_bytes = term_bytes;
	LIST_NODE_CONS(new_split->ln);

	list_insert_one_at_tail(&new_split->ln, sa->into_list, NULL, NULL);

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node,
                  ln, free(p));

LIST_DEF_FREE_FUN(release_splited_terms, struct splited_term,
                  ln, free(p));

#define DEBUG_LEX_ENG_ONLY /* uncomment to index Chinese */

#ifndef DEBUG_LEX_ENG_ONLY
void split_into_terms(struct lex_slice *slice, list *ret)
{
	struct split_arg sa;
	list   seg_li = LIST_NULL;

	sa.into_list = ret;
	sa.slice = slice;
	sa.slice_chars = term_offset_conv_init(slice->mb_str);

	seg_li = text_segment(slice->mb_str);
	list_foreach(&seg_li, &conv_pos, &sa);

	term_offset_conv_uninit();
	list_release(&seg_li);
}
#else
void split_into_terms(struct lex_slice *slice, list *ret)
{
	struct splited_term *unchange;
	unchange = malloc(sizeof(struct splited_term));
	strcpy(unchange->term, slice->mb_str);
	unchange->doc_pos = slice->begin;
	unchange->n_bytes = slice->offset;
	LIST_NODE_CONS(unchange->ln);

	list_insert_one_at_tail(&unchange->ln, ret, NULL, NULL);
}
#endif

#include <ctype.h> /* for tolower() */

void eng_to_lower_case(struct lex_slice *slice)
{
	size_t i;
	for(i = 0; i < slice->offset; i++)
		slice->mb_str[i] = tolower(slice->mb_str[i]);
}

#ifdef DEBUG_LEX_ENG_ONLY

#define  LEX_PREFIX(_name) eng ## _name
#include "lex.h"

#else

#define  LEX_PREFIX(_name) mix ## _name
#include "lex.h"

#endif

void lex_txt_file(const char *path)
{
	FILE *fh = fopen(path, "r");
	if (fh) {
		LEX_PREFIX(in) = fh;
		DO_LEX;
		fclose(fh);
	} else {
		fprintf(stderr, "cannot open `%s'.\n", path);
	}
}

struct position_node {
	uint32_t begin, offset /* in bytes */;
	struct list_node ln;
};

static list  file_pos_check_list = LIST_NULL;
uint32_t     file_pos_check_cnt  = 0;
static FILE *file_pos_check_fh   = NULL;

static char bigbuf[MAX_TERM_BYTES + 4096 /* protect padding */];

static LIST_IT_CALLBK(print_position)
{
	LIST_OBJ(struct position_node, p, ln);
	P_CAST(fh, FILE, pa_extra);
	size_t n_read;

	printf("checking #%u pos", file_pos_check_cnt++);
	printf("<%u, %u>: ", p->begin, p->offset);

	if (0 == fseek(fh, p->begin, SEEK_SET)) {
		n_read = fread(&bigbuf, p->offset, 1, fh);
		if (n_read == 1) {
			bigbuf[p->offset] = '\0';
			printf("{%s}\n", bigbuf);
		} else {
			printf("unexpected test offset %lu-%u.\n",
			       n_read, p->offset);
			return LIST_RET_BREAK;
		}
	} else {
		printf("bad fseek position.\n");
		return LIST_RET_BREAK;
	}

	LIST_GO_OVER;
}

void file_pos_check_print()
{
	if (file_pos_check_fh) {
		file_pos_check_cnt = 0;
		list_foreach(&file_pos_check_list, &print_position,
		             file_pos_check_fh);
	}
}

int file_pos_check_init(const char *fname)
{
	file_pos_check_fh = fopen(fname,"r");
	if (file_pos_check_fh == NULL) {
		fprintf(stderr, "cannot open test file for position checking.\n");
		return 1;
	}

	LIST_CONS(file_pos_check_list);
	file_pos_check_cnt = 0;
	return 0;
}

LIST_DEF_FREE_FUN(free_positions_list, struct position_node,
                  ln, free(p));

void file_pos_check_free()
{
	if (file_pos_check_fh)
		fclose(file_pos_check_fh);

	free_positions_list(&file_pos_check_list);
}

void file_pos_check_add(size_t begin, size_t offset)
{
	struct position_node *nd = malloc(sizeof(struct position_node));
	LIST_NODE_CONS(nd->ln);
	nd->begin = begin;
	nd->offset = offset;
	list_insert_one_at_tail(&nd->ln, &file_pos_check_list, NULL, NULL);
	file_pos_check_cnt ++;
}
