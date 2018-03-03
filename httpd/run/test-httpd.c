#include "mhook/mhook.h"
#include "httpd.h"

static const char *hello_on_recv(const char* req, void* arg_)
{
	unsigned short *port = (unsigned short*)arg_;
	printf("hello: %s @ %u \n\n", req, *port);
	return req;
}

static const char *world_on_recv(const char* req, void* arg_)
{
	unsigned short *port = (unsigned short*)arg_;
	printf("world: %s @ %u \n\n", req, *port);
	return req;
}

int main()
{
	unsigned short port = 8914;
	struct uri_handler uri_handlers[] = {
		{"/hello" , hello_on_recv},
		{"/world" , world_on_recv}
	};
	unsigned int len = sizeof(uri_handlers) / sizeof(struct uri_handler);

	printf("listening at port %u ...\n", port);
	httpd_run(port, uri_handlers, len, &port);

	mhook_print_unfree();
	return 0;
}
