#include "mhook/mhook.h"
#include "math-index.h"

int main()
{
	/* simple test */
//	const char *test[] = {
//		"ab",             // expID = 0
//		"a+k(b+c)",       // expID = 1
//		"a+bc+xy",        // expID = 2
//		"a(b+cd)",        // expID = 3
//		"\\sqrt{pq + m}", // expID = 4
//	};

	/* prefix match test */
//	const char *test[] = {
//		"2 \\sum_{k=0}^{r-1} \\binom{n}{2k+1} \\binom{n}{2r-2k-1} &= \\binom{2n}{2r} - (-1)^k \\binom{n}{r}"
//	};

	/* joint node constraints test */
	const char *test[] = {
		"abcd + 1",
		"a+bc+xy",
		"ab + cd"
	};

//	/* extreme-condition test */
//	const char *test[] = {
//		"abcdefghijklmnopqrstuvwxyz" /* 26 */
//		"abcdefghijklmnopqrstuvwxyz" /* 52 */
//		"abcdefghijkl" /* 64 */
//	};

	/* joint node constraints test 2 */
//	const char *test[] = {
//"2+\\cfrac{3}{4+\\cfrac{5}{6+\\cfrac{7}{8 +\\ddots}}}"
//	};

	doc_id_t docID = 1;
	exp_id_t expID = 0;
	uint32_t i, n_test = sizeof(test)/sizeof(char*);

	struct tex_parse_ret parse_ret;
	math_index_t index = math_index_open("./tmp", MATH_INDEX_WRITE);

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	for (i = 0; i < n_test; i++) {
		printf("add: `%s' as experssion#%u\n", test[i], expID);
		parse_ret = tex_parse(test[i], 0, false);

		if (parse_ret.code != PARSER_RETCODE_ERR) {
			subpaths_print(&parse_ret.subpaths, stdout);
			math_index_add_tex(index, docID, expID, parse_ret.subpaths);
			subpaths_release(&parse_ret.subpaths);
			expID ++;
		} else {
			printf("parser error: %s\n", parse_ret.msg);
		}
	}

	math_index_close(index);

	mhook_print_unfree();
	return 0;
}
