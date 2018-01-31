#include "mhook/mhook.h"
#include "math-index.h"

int main()
{
	/* normal test */
	const char *test[] = {
		"ab",             // expID = 0
		"a+k(b+c)",       // expID = 1
		"a+bc+xy",        // expID = 2
		"a(b+cd)",        // expID = 3
		"\\sqrt{pq + m}"  // expID = 4
	};

	printf("%s\n", test[0]);

// 	doc_id_t docID = 1;
// 	exp_id_t expID = 0;
// 	uint32_t i, n_test = sizeof(test)/sizeof(char*);
// 
// 	struct tex_parse_ret parse_ret;
// 	math_index_t index = math_index_open("./tmp", MATH_INDEX_WRITE);
// 
// 	if (index == NULL) {
// 		printf("cannot open index.\n");
// 		return 1;
// 	}
// 
// 	for (i = 0; i < n_test; i++) {
// 		printf("add: `%s' as experssion#%u\n", test[i], expID);
// 		parse_ret = tex_parse(test[i], 0, false);
// 
// 		if (parse_ret.code != PARSER_RETCODE_ERR) {
// 			subpaths_print(&parse_ret.subpaths, stdout);
// 			//math_index_add_tex(index, docID, expID, parse_ret.subpaths);
// 			subpaths_release(&parse_ret.subpaths);
// 			expID ++;
// 		} else {
// 			printf("parser error: %s\n", parse_ret.msg);
// 		}
// 	}
// 
// 	math_index_close(index);
// 
// 	mhook_print_unfree();
	return 0;
}
