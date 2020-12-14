#include <stdio.h>
#include "mhook/mhook.h"
#include "tex-parser/head.h"
#include "math-index.h"

int main()
{
	/* prefix match test */
//	const char *test[] = {
//		"\\qvar{a} + \\qvar{b}"
//		"a+ba+ab +\\sqrt z + g/h + (ij + k) \\le 0"
//	};

	/* joint node constraints test */
//	const char *test[] = {
//		"abcd + 1",
//		"a+bc+xy",
//		"ab + cd"
//	};

//	/* extreme-condition test */
//	const char *test[] = {
//		"abcdefghijklmnopqrstuvwxyz" /* 26 */
//		"abcdefghijklmnopqrstuvwxyz" /* 52 */
//		"abcdefghijkl" /* 64 */
//	};

//	/* joint node constraints test 2 */
//	const char *test[] = {
//		"2+\\cfrac{3}{4+\\cfrac{5}{6+\\cfrac{7}{8 +\\ddots}}}"
//	};

	/* default test */
	const char *test[] = {
//		"ab",             // expID = 0
//		"a+k(b+c)",       // expID = 1
//		"a+bc+xy",        // expID = 2
//		"a(b+cd)",        // expID = 3
//		"\\sqrt{pq + m}", // expID = 4
//		"pq + 1",         // expID = 5
//		"x + x + x +z + z + (a + a + b) c", // ...
		"a + a + b/c + d ="
	};

	uint32_t docID = 1;
	uint32_t expID = 0;

	struct tex_parse_ret parse_ret;
	math_index_t index = math_index_open("./tmp", "w");

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	// for (int n = 0; n < 200; n++) {
	for (int n = 0; n < 1; n++) {
	for (int i = 0; i < sizeof(test) / sizeof(char*); i++) {
		printf("add: `%s' as experssion#%u\n", test[i], expID);

#ifdef SUPPORT_MATH_WILDCARDS
#error("math wildcards not supported yet.")
#else
		parse_ret = tex_parse(test[i]);
#endif

		if (parse_ret.code != PARSER_RETCODE_ERR) {
			/* print operator tree */
			optr_print(parse_ret.operator_tree, stdout);
			optr_release(parse_ret.operator_tree);

			/* print leaf-root paths */
			subpaths_print(&parse_ret.lrpaths, stdout);

			/* index the tex */
			size_t flush_sz =
			math_index_add(index, docID, expID, parse_ret.lrpaths);
			prinfo("flush %lu byte(s)\n", flush_sz);
			subpaths_release(&parse_ret.lrpaths);
			expID ++;
		} else {
			printf("parser error: %s\n", parse_ret.msg);
		}
	}
	}

	math_index_flush(index);
	math_index_print(index);

	math_index_close(index);

	mhook_print_unfree();
	return 0;
}
