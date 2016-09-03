#include <stdio.h>
#include <assert.h>

#include "mhook/mhook.h"
#include "mem-index/mem-posting.h"
#include "indices.h"

int main(void)
{
	const char index_path[] = "../indexer/tmp";
	struct indices indices;

	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("open index failed.\n");
		goto exit;
	}

	indices_cache(&indices, 30 MB);

exit:
	printf("closing index...\n");
	indices_close(&indices);

	mhook_print_unfree();
	return 0;
}
