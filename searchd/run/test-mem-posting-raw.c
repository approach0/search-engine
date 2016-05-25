#include <stdio.h>
#include <stdlib.h>
#include "mem-posting.h"

int main(void)
{
	size_t i, j, n;
	size_t len;
	struct mem_posting *po = NULL;
	uint32_t test_data[MEM_POSTING_BLOCK_SZ];
	uint32_t docID = 0;

	po = mem_posting_create(2);
	printf("%u bytes/block.\n", MEM_POSTING_BLOCK_SZ);

	for (n = 0; n < 6; n++)
	for (i = 1; i < 10; i++) {
		/* write a simulated buffer `test_data' of size `sz' */
		docID = docID + i;
		len = 5 + i * 10;
		for (j = 0; j < len; j++) {
			if (j + 1 == len)
				test_data[j] = 0;
			else
				test_data[j] = i;
		}

		printf("writing docID#%u (length %lu)\n", docID, len);
		mem_posting_write(po, docID, test_data, len * sizeof(uint32_t));
	}

	mem_posting_print(po);
	mem_posting_release(po);
	return 0;
}
