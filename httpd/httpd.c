#include <stdlib.h>
#include <stdio.h>
#include <evhttp.h>
#include <signal.h>

#include "httpd.h"

struct http_cb_arg {
	httpd_on_recv_cb on_recv;
	void *arg;
};

static void signal_handler(int sig) {
	switch (sig) {
		case SIGTERM:
		case SIGHUP:
		case SIGQUIT:
		case SIGINT:
			event_loopbreak();
			break;
	}
}

static char *get_POST_str(struct evhttp_request *req)
{
	struct evbuffer *buf;
	size_t len;
	char  *cbuf;

	if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
		return NULL;
	}

	buf = evhttp_request_get_input_buffer(req);
	len = evbuffer_get_length(buf);

	if (len == 0)
		return NULL;

	cbuf = malloc(len + 1);

	if (cbuf == NULL)
		return NULL;

	evbuffer_remove(buf, cbuf, len);
	cbuf[len] = '\0';
	return cbuf;
}

static void
httpd_callbk(struct evhttp_request *req, void *arg_)
{
	struct http_cb_arg *arg = (struct http_cb_arg *)arg_;
	struct evbuffer *buf;
	char *request;
	const char *response;

	/* create HTTP response buffer */
	buf = evbuffer_new();

	request = get_POST_str(req);
	if (request != NULL) {
		response = arg->on_recv(request, arg->arg);
	} else {
		fprintf(stderr, "httpd: POST data is NULL.\n");
		goto reply;
	}

	/* set HTTP response header */
	evhttp_add_header(req->output_headers, "Content-Type",
	                  "application/json; charset=UTF-8");
	evhttp_add_header(req->output_headers, "Connection",
	                  "close");

	/* set HTTP response content */
	if (response)
		evbuffer_add_printf(buf, "%s", response);

	/*
	 * Free request after response is copied (in case they
	 * have the same pointer address, e.g. an echo server)
	 */
	free(request);

reply:
	/* send response */
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}

int httpd_run(unsigned short port,
              struct uri_handler *handlers,
              unsigned int n_handlers,
              void *arg)
{
	struct evhttp *httpd;
	struct http_cb_arg *copy_args;

	/* initialization */
	signal(SIGINT, signal_handler);
	event_init();

	/* binding */
	httpd = evhttp_start("0.0.0.0", port);

	/* check if port is already binded */
	if (httpd == NULL) {
		fprintf(stderr, "Failed to listen on port %d.\n", port);
		return 1;
	}

	/* allocate handler list for each callback instance */
	copy_args = malloc(sizeof(struct http_cb_arg) * n_handlers);

	/* set callback functions on receving */
	for (int i = 0; i < n_handlers; i++) {
		copy_args[i].on_recv = handlers[i].handler;
		copy_args[i].arg = arg;
		// printf("setup URI handler for %s ...\n", handlers[i].uri);
		evhttp_set_cb(httpd, handlers[i].uri, httpd_callbk, copy_args + i);
	}

	/* main loop */
	event_dispatch();

	/* closing */
	printf("\n");
	printf("shutdown httpd...\n");

	free(copy_args);
	evhttp_free(httpd);

	return 0;
}
