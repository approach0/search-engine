#include "mhook/mhook.h"
#include "math-index.h"
#include "subpath-set.h"

int main()
{
	/* normal test */
	const char *test[] = {
		"a/b",
		"a+k(b+c)",
		"a+bc+xy",
		"2 \\sum_{k=0}^{r-1} \\binom{n}{2k+1} \\binom{n}{2r-2k-1} &= \\binom{2n}{2r} - (-1)^k \\binom{n}{r} "
	};

 	struct tex_parse_ret parse_ret;
 	uint32_t i, n_test = sizeof(test)/sizeof(char*);

	struct subpath_ele_added added;
	list subpath_set = LIST_NULL;

	for (i = 0; i < n_test; i++) {
		printf("test: %s\n", test[i]);
		parse_ret = tex_parse(test[i], 0, false);

		if (parse_ret.code != PARSER_RETCODE_ERR) {
			printf("%u subpaths:\n", parse_ret.subpaths.n_subpaths);
			subpaths_print(&parse_ret.subpaths, stdout);

			added =
				prefix_subpath_set_from_subpaths(&parse_ret.subpaths, &subpath_set);

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
