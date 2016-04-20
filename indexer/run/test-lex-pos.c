#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wstring/wstring.h"

/* below two are for MAX_TERM_BYTES */
#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"

#include "list/list.h"

#define  LEX_PREFIX(_name) txt ## _name
#include "lex.h"

extern size_t lex_seek_pos;

list positions_list = LIST_NULL;
uint32_t cnt_positions = 0;

struct position_node {
	size_t begin, offset /* in bytes */;
	struct list_node ln;
};

static LIST_IT_CALLBK(print_position)
{
	LIST_OBJ(struct position_node, p, ln);
	P_CAST(fh, FILE, pa_extra);
	char term[MAX_TERM_BYTES];

	printf("#%u ", ++cnt_positions);
	printf("<%lu, %lu>: ", p->begin, p->offset);

	fseek(fh, p->begin, SEEK_SET);
	fread(&term, p->offset, 1, fh);
	term[p->offset] = '\0';
	printf("%s\n", term);

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(free_positions_list, struct position_node,
                  ln, free(p));

static void insert_pos(size_t begin, size_t offset)
{
	struct position_node *nd = malloc(sizeof(struct position_node));
	LIST_NODE_CONS(nd->ln);
	nd->begin = begin;
	nd->offset = offset;
	list_insert_one_at_tail(&nd->ln, &positions_list, NULL, NULL);
}

int handle_chinese(char *term)
{
	size_t   len = strlen(term); /* in bytes */
	printf("#%u ", ++cnt_positions);
	printf("Chinese word:`%s' [byte pos, offset] = [%lu, %lu].\n",
	       term, lex_seek_pos - len, len);
	insert_pos(lex_seek_pos - len, len);

	return 0;
}

int handle_english(char *term)
{
	size_t   len = strlen(term); /* in bytes */

	printf("#%u ", ++cnt_positions);
	printf("English word:`%s' [byte pos, offset] = [%lu, %lu].\n",
	       term, lex_seek_pos - len, len);
	insert_pos(lex_seek_pos - len, len);
	return 0;
}

static void lexer_file_input(const char *path)
{
	FILE *fh = fopen(path, "r");
	if (fh) {
		txtin = fh;
		txtlex();
		fclose(fh);
	} else {
		printf("cannot open `%s'.\n", path);
	}
}

int main()
{
	FILE *fh;

	lexer_file_input("test.tmp");

	fh = fopen("test.tmp","r");
	if (fh == NULL)
		goto free;

	cnt_positions = 0;
	list_foreach(&positions_list, &print_position, fh);

	fclose(fh);
free:
	free_positions_list(&positions_list);
}
