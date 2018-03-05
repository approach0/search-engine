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

	qac_index_t *qi = qac_index_open("./tmp", QAC_INDEX_WRITE_READ);

	for (i = 0; i < n_test; i++) {
		uint32_t texID = math_qac_index_uniq_tex(qi, test[i]);
		printf("texID#%u: `%s'\n", texID, test[i]);
	}
	printf("\n");

	for (i = 0; i < 3; i++) {
		struct qac_tex_info tex_info;
		char *tex;
		printf("getting ID=%u ...\n", i);
		tex_info = math_qac_get(qi, i, &tex);
		if (tex) {
			printf("%s (freq=%u)\n", tex, tex_info.freq);
			free(tex);
		} else {
			printf("No such ID.\n");
		}
	}

	qac_index_close(qi);
	mhook_print_unfree();
	return 0;
}
