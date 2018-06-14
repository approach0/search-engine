#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mhook/mhook.h"
#include "linkli/list.h"
#include "tex-parser/vt100-color.h"

#define prerr(_fmt, ...) \
	fprintf(stderr, C_RED "Error: " C_RST _fmt "\n", ##__VA_ARGS__);

#define prinfo(_fmt, ...) \
	fprintf(stderr, C_BLUE "Info: " C_RST _fmt "\n", ##__VA_ARGS__);

struct A {
	int docID;
	int tf;
	int pos_arr[64];
};

void *test_field_ptr(void*, int);
char *test_field_info(int);
int   test_field_size(void*, int);

struct postlist_codec_args {
	int   size;
	int   n_fields;
	void *(*field_ptr)(void*, int);
	int   (*field_size)(void*, int);
	char *(*field_info)(int);
	int   (*field_codec)(int);
};

void
postlist_print_item_struct(void *inst, struct postlist_codec_args args)
{
	for (int j = 0; j < args.n_fields; j++) {
		prinfo("Field [%s]", args.field_info(j));
		prinfo("offset: %lu",
		       (uintptr_t)args.field_ptr(inst, j) - (uintptr_t)inst);
		prinfo("size: %d", args.field_size(inst, j));
	}
}

void*
postlist_random(int n, struct postlist_codec_args args)
{
	void *po = malloc(n * args.size);
	srand(time(0));
	for (int i = 0; i < n; i++) {
		void *cur = (char *)po + i * args.size;
		//printf("doc[%d]: \n", i);
		for (int j = 0; j < args.n_fields; j++) {
			int *p = (int *)args.field_ptr(cur, j);
			for (int k = 0; k < args.field_size(cur, j) / sizeof(int); k++) {
				p[k] = rand() % 10 + 1;
				//printf("field[%d][%d] = %d \n", j, k, p[k]);
			}
		}
	}

	return po;
}

void
postlist_print(void *po, int n, struct postlist_codec_args args)
{
	for (int i = 0; i < n; i++) {
		void *cur = (char *)po + i * args.size;
		for (int j = 0; j < args.n_fields; j++) {
			int *p = (int *)args.field_ptr(cur, j);
			int field_len = args.field_size(cur, j) / sizeof(int);
			printf("[");
			for (int k = 0; k < field_len; k++) {
				printf("%d", p[k]);
				if (k + 1 != field_len) {
					printf(", ");
				}
			}
			printf("]");
		}
		printf("\n");
	}
}

int main()
{
	struct postlist_codec_args args = {
		sizeof(struct A), 3, test_field_ptr,
		test_field_size, test_field_info, NULL
	};

	struct A foo = {12, 3, {1, 2, 3}};
	postlist_print_item_struct(&foo, args);

	struct A *doc = postlist_random(5, args);
	postlist_print(doc, 5, args);
	free(doc);

	return 0;
}

void *test_field_ptr(void *inst, int j)
{
	char *base = (char *)inst;
	switch (j) {
	case 0:
		return base + offsetof(struct A, docID);
	case 1:
		return base + offsetof(struct A, tf);
	case 2:
		return base + offsetof(struct A, pos_arr);
	default:
		prerr("Unexpected field number");
		abort();
	}
}

int test_field_size(void *inst, int j)
{
	struct A *a = (struct A*)inst;
	switch (j) {
	case 0:
		return sizeof(a->docID);
	case 1:
		return sizeof(a->tf);
	case 2:
		return a->tf * sizeof(__typeof__(a->pos_arr[0]));
	default:
		prerr("Unexpected field number");
		abort();
	}
}

char *test_field_info(int j)
{
	switch (j) {
	case 0:
		return "doc ID";
	case 1:
		return "term freq";
	case 2:
		return "positions";
	default:
		prerr("Unexpected field number");
		abort();
	}
}
