#include <stdio.h>
#include "indexer.h"
#include "yajl/yajl_tree.h"

void lex_slice_handler(struct lex_slice *slice)
{
	return;
}

int main()
{
	const char json[] = "{\"drink\" : {\"name\" : \"可口可乐 \\\"Diet\\\"\"}}";
	const char *path[] = {"drink", "name", NULL};
	char errstr[1024] = {0};
	yajl_val tr, node;

	printf("JSON string: %s\n", json);
	tr = yajl_tree_parse(json, errstr, sizeof(errstr));

	if (tr == NULL) {
		fprintf(stderr, "parser error: %s\n", errstr);
		return 1;
	}

	node = yajl_tree_get(tr, path, yajl_t_string);

	if (node) {
		printf("%s\n", YAJL_GET_STRING(node));
	} else {
		printf("no such JSON node.\n");
	}

	yajl_tree_free(tr);
	return 0;
}
