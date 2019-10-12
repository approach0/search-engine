#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "httpd/httpd.h"

#include "config.h"
#include "indices-v3/indices.h"

static int index_maintain(struct indices *indices)
{
	/* print maintainer related stats */
	const int64_t unfree = mhook_unfree();
	printf(ES_RESET_LINE "Unfree: %ld / %u, Size: %lu / %u",
		unfree, MAINTAIN_UNFREE_CNT,
		term_index_size(indices->ti), MAXIMUM_INDEX_COUNT
	);
	fflush(stdout);

	if (index_should_maintain()) {
		printf(ES_RESET_LINE "[index maintaining...]");
		fflush(stdout);
		index_maintain();

	} else if (unfree > UNFREE_CNT_INDEXER_MAINTAIN) {
		printf(ES_RESET_LINE "[index write onto disk...]");
		fflush(stdout);
		index_write();
	}
	return 0;
}

static const char *httpd_on_index(const char* req, void* arg_)
{
	P_CAST(indices, struct indices, arg_);
	index_maintain(indices);

	printf("%s\n", req);
	return req;
}

int main()
{
	struct uri_handler handlers[] = {
		{INDEXD_DEFAULT_URI, &httpd_on_index}
	};
	struct indices indices;
	struct indexer *indexer;
*indexer_alloc(struct indices *indices, text_lexer lexer,
               parser_exception_callbk on_exception)
	unsigned short port = INDEXD_DEFAULT_PORT;

	indices_open(&indices, "./tmp", INDICES_OPEN_RW);

	printf("listening at %u:%s ...\n", port, handlers[0].uri);
	httpd_run(port, handlers,
		sizeof handlers / sizeof(struct uri_handler), indices);

	printf("closing index...\n");
	indices_close(&indices);

	mhook_print_unfree();
	return 0;
}
