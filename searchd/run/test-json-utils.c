#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "json-utils.h"

static char *read_file(const char *path)
{
	char *ret_str;
	size_t file_sz;
	FILE *fd = fopen(path, "r");

	if (fd == NULL)
		return NULL;

	/* get file size */
	fseek(fd, 0, SEEK_END);
	file_sz = ftell(fd);
	rewind(fd);

	/* read file */
	ret_str = malloc(file_sz + 1);
	(void)fread(ret_str, 1, file_sz, fd);
	fclose(fd);

	ret_str[file_sz] = '\0';
	return ret_str;
}

static void test_json_file(const char *path)
{
	struct query qry = QUERY_NEW;
	char *test_req = read_file(path);
	uint32_t page;

	if (test_req == NULL) {
		fprintf(stderr, "No such file: %s.\n", path);
		return;
	}

	printf("testing JSON request:\n");
	printf("%s\n", test_req);
	printf("EOF\n");

	page = parse_json_qry((const char *)test_req, &qry);

	if (page != 0) {
		printf("page number = %u.\n", page);
		printf("parsed query: \n");
		query_print(qry, stdout);
		printf("\n");
	}

	free(test_req);
	query_delete(qry);

	printf("\n=======================\n");
}

int main()
{
	static char test_enc[1024];
	json_encode_str(test_enc, "\"hello world\".", 1024);
	printf("encoded string: %s\n", test_enc);

	test_json_file("./tests/query-bad-json.json");
	test_json_file("./tests/query-bad-key-type.json");
	test_json_file("./tests/query-bad-key.json");
	test_json_file("./tests/query-bad-val.json");
	test_json_file("./tests/query-lack-key.json");
	test_json_file("./tests/query-no-hits.json");
	test_json_file("./tests/query-no-page.json");
	test_json_file("./tests/query-valid.json");

	mhook_print_unfree();
	return 0;
}
