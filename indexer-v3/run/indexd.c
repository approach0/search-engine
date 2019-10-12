#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "httpd/httpd.h"

#include "config.h"
#include "indices-v3/indices.h"

static const char *httpd_on_index(const char* req, void* arg_)
{
	printf("%s\n", req);
	return req;
}

int main()
{
	struct uri_handler handlers[] = {
		{INDEXD_DEFAULT_URI, &httpd_on_index}
	};
	struct indices indices;
	unsigned short port = INDEXD_DEFAULT_PORT;

	indices_open(&indices, "./tmp", INDICES_OPEN_RW);

	printf("listening at %u:%s ...\n", port, handlers[0].uri);
	httpd_run(port, handlers,
		sizeof handlers / sizeof(struct uri_handler), NULL);

	printf("closing index...\n");
	indices_close(&indices);

	mhook_print_unfree();
	return 0;
}
