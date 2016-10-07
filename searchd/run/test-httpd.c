#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "timer/timer.h"

#include "config.h"
#include "httpd.h"

static const char *httpd_on_recv(const char* req, void* arg_)
{
	printf("recv a request, wait 5 sec...\n");
	delay(5, 0, 0);
	printf("%s.\n", req);

	return req;
}

int main()
{
	printf("listening...\n");
	httpd_run(8921, &httpd_on_recv, NULL);

	mhook_print_unfree();
	return 0;
}
