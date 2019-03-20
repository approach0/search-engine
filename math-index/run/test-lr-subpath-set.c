#include "mhook/mhook.h"
#include "math-index.h"
#include "subpath-set.h"

int main()
{
	const char *test[] = {"ab", "a+k(b+c)", "k(b+c)+a"};
	uint32_t i, n_test = sizeof(test)/sizeof(char*);
	struct tex_parse_ret parse_ret;
	struct subpath_ele_added added;
	list subpath_set = LIST_NULL;

	for (i = 0; i < n_test; i++) {
		printf("test: %s\n", test[i]);
		parse_ret = tex_parse(test[i], 0, false, false);

		if (parse_ret.code != PARSER_RETCODE_ERR) {
			printf("%u subpaths:\n", parse_ret.subpaths.n_subpaths);
			subpaths_print(&parse_ret.subpaths, stdout);

			added =
				lr_subpath_set_from_subpaths(&parse_ret.subpaths, &subpath_set);

			printf("%u unique subpaths:\n", added.new_uniq);
			subpath_set_print(&subpath_set, stdout);
			subpath_set_free(&subpath_set);

			subpaths_release(&parse_ret.subpaths);
			printf("\n");
		} else {
			printf("parser error: %s\n", parse_ret.msg);
		}
	}

	mhook_print_unfree();
	return 0;
}
