#include "mhook/mhook.h"
#include "math-index.h"
#include "subpath-set.h"

int main()
{
	/* normal test */
	const char *test[] = {
		"a+k(b+c)", 
		"a+bc+xy"
	};

	printf("%s\n", test[0]);

// 	doc_id_t docID = 1;
// 	exp_id_t expID = 0;
 	struct tex_parse_ret parse_ret;
 	uint32_t i, n_test = sizeof(test)/sizeof(char*);

	uint32_t n_uniq;
	list subpath_set = LIST_NULL;
// 
// 	math_index_t index = math_index_open("./tmp", MATH_INDEX_WRITE);
// 
// 	if (index == NULL) {
// 		printf("cannot open index.\n");
// 		return 1;
// 	}
// 
	for (i = 0; i < n_test; i++) {
		printf("test: %s\n", test[i]);
		parse_ret = tex_parse(test[i], 0, false);

		if (parse_ret.code != PARSER_RETCODE_ERR) {
			delete_gener_paths(&parse_ret.subpaths);
			printf("%u subpaths:\n", parse_ret.subpaths.n_subpaths);
			subpaths_print(&parse_ret.subpaths, stdout);

			n_uniq = subpath_set_from_subpaths(&parse_ret.subpaths,
			                                   &sp_prefix_comparer,
			                                   &subpath_set);

			printf("%u unique subpaths:\n", n_uniq);
			subpath_set_print(&subpath_set, stdout);
			subpath_set_free(&subpath_set);

			subpaths_release(&parse_ret.subpaths);
			printf("\n");
		} else {
			printf("parser error: %s\n", parse_ret.msg);
		}
	}
// 	math_index_close(index);
// 
 	mhook_print_unfree();
	return 0;
}
