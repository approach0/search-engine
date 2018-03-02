#include "mhook/mhook.h"
#include "qac.h"

int main()
{
	const char *test[] = {
		"abcd + 1",
		"a+bc+xy",
		"xyz + 1",
		"wxyz + 1"
	};

	uint32_t i, n_test = sizeof(test)/sizeof(char*);
	exp_id_t expID = 1;

	math_index_t index = math_index_open("./tmp", MATH_INDEX_WRITE);

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	for (i = 0; i < n_test; i++) {
		printf("try to add: `%s' as experssion#%u\n", test[i], expID);
		math_qac_index_uniq_tex(index, test[i], 1, expID);
		expID ++;
	}

	math_index_close(index);

	mhook_print_unfree();
	return 0;
}
