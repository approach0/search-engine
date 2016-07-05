#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "txt-seg/txt-seg.h" /* for MAX_TERM_BYTES */
#include "list/list.h"
#include "offset-check.h"

struct check_node {
	uint32_t offset, n_bytes /* in bytes */;
	struct list_node ln;
};

static list  file_offset_check_list = LIST_NULL;
uint32_t     file_offset_check_cnt  = 0;
static FILE *file_offset_check_fh   = NULL;

static char bigbuf[MAX_TXT_SEG_BYTES + 4096 /* protect padding */];

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct check_node, p, ln);
	P_CAST(fh, FILE, pa_extra);
	size_t n_read;

	printf("checking #%u offset", file_offset_check_cnt++);
	printf("<%u, %u>: ", p->offset, p->n_bytes);

	if (0 == fseek(fh, p->offset, SEEK_SET)) {
		n_read = fread(&bigbuf, p->n_bytes, 1, fh);
		if (n_read == 1) {
			bigbuf[p->n_bytes] = '\0';
			printf("{%s}\n", bigbuf);
		} else {
			printf("unexpected n_read: %lu-%u.\n",
			       n_read, p->n_bytes);
			return LIST_RET_BREAK;
		}
	} else {
		printf("bad fseek position.\n");
		return LIST_RET_BREAK;
	}

	LIST_GO_OVER;
}

void file_offset_check_print()
{
	if (file_offset_check_fh) {
		file_offset_check_cnt = 0;
		list_foreach(&file_offset_check_list, &print,
		             file_offset_check_fh);
	}
}

int file_offset_check_init(const char *fname)
{
	file_offset_check_fh = fopen(fname,"r");
	if (file_offset_check_fh == NULL) {
		fprintf(stderr, "cannot open test file for checking.\n");
		return 1;
	}

	LIST_CONS(file_offset_check_list);
	file_offset_check_cnt = 0;
	return 0;
}

LIST_DEF_FREE_FUN(free_check_list, struct check_node,
                  ln, free(p));

void file_offset_check_free()
{
	if (file_offset_check_fh)
		fclose(file_offset_check_fh);

	free_check_list(&file_offset_check_list);
}

void file_offset_check_add(uint32_t offset, uint32_t n_bytes)
{
	struct check_node *nd = malloc(sizeof(struct check_node));
	LIST_NODE_CONS(nd->ln);
	nd->offset = offset;
	nd->n_bytes = n_bytes;
	list_insert_one_at_tail(&nd->ln, &file_offset_check_list, NULL, NULL);
	file_offset_check_cnt ++;
}
