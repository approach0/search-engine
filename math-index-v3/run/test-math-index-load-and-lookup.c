#include <stdio.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "math-index.h"

static void test_lookup(math_index_t index, const char *path_key)
{
	struct math_invlist_entry_reader entry_reader;
	printf("\n");
	printf("look up key: %s\n", path_key);
	entry_reader = math_index_lookup(index, path_key);
	if (entry_reader.pf) {
		printf("pf = %u, type = %s\n", entry_reader.pf,
			(entry_reader.medium == MATH_READER_MEDIUM_INMEMO) ?
			"in-memory" : "on-disk");
		invlist_iter_print_as_decoded_ints(entry_reader.reader);
		invlist_iter_free(entry_reader.reader);
		fclose(entry_reader.fh_symbinfo);
	} else {
		printf("not existing.\n");
	}
	printf("\n");
}

int main()
{
	math_index_t index = math_index_open("./tmp", "r");

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	math_index_load(index, 20 KB);
	printf("load finished.\n");

	math_index_print(index);

	{
		test_lookup(index, "/prefix/VAR/ADD");
		test_lookup(index, "/prefix/VAR/ADD/GROUP/TIMES/ADD");
		test_lookup(index, "/NON/EXISTS");
	}

	printf("closing ...\n");
	math_index_close(index);

	mhook_print_unfree();
	return 0;
}
