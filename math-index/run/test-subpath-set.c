#include "math-index.h"
#include "subpath-set.h"

struct math_posting_item dummy;

static LIST_IT_CALLBK(add_into_set)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(set, list, pa_extra);

	subpath_set_add(set, sp, &dummy);
	LIST_GO_OVER;
}

int main()
{
	const char *test[] = {"ab", "a+k(b+c)", "k(b+c)+a"};
	uint32_t i, n_test = sizeof(test)/sizeof(char*);
	struct tex_parse_ret parse_ret;
	list subpath_set = LIST_NULL;

	for (i = 0; i < n_test; i++) {
		printf("test: %s\n", test[i]);
		parse_ret = tex_parse(test[i], 0, false);

		if (parse_ret.code == PARSER_RETCODE_SUCC) {
			printf("%u subpaths in total:\n", parse_ret.subpaths.n_subpaths);
			subpaths_print(&parse_ret.subpaths, stdout);

			list_foreach(&parse_ret.subpaths.li, &add_into_set, &subpath_set);
			printf("unique subpaths:\n");
			subpath_set_print(&subpath_set, stdout);
			subpath_set_free(&subpath_set);

			subpaths_release(&parse_ret.subpaths);
			printf("\n");
		} else {
			printf("parser error: %s\n", parse_ret.msg);
		}
	}

	return 0;
}
