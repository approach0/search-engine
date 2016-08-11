#include <stdlib.h>
#include <stdio.h>
#include <evhttp.h>
#include <signal.h>

#undef N_DEBUG
#include <assert.h>

#include "config.h"
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

static void print_headers(struct evhttp_request *req)
{
	struct evkeyvalq *headers;
	struct evkeyval  *header;

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header != NULL;
	     header = header->next.tqe_next) {
		 printf("%s: %s\n", header->key, header->value);
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
	char *request, *response;

#ifdef DEBUG_PRINT_HTTP_HEAD
	printf("HTTP request header:\n");
	print_headers(req);
#endif

	/* create HTTP response buffer */
	buf = evbuffer_new();
	assert(buf != NULL);

	request = get_POST_str(req);
	if (request != NULL) {
		response = arg->on_recv(request, arg->arg);
		free(request);
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
	if (response) {
		evbuffer_add_printf(buf, "%s", response);
		free(response);
	}

reply:
	/* send response */
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);
}

int httpd_run(unsigned short port,
              httpd_on_recv_cb on_recv, void *arg)
{
	struct evhttp *httpd;
	struct http_cb_arg callbk_arg = {on_recv, arg};

	/* initialization */
	signal(SIGINT, signal_handler);
	event_init();

	/* binding */
	httpd = evhttp_start("0.0.0.0", port);

	evhttp_set_cb(httpd, SEARCHD_DEFAULT_URI,
	              httpd_callbk, &callbk_arg);

	/* main loop */
	event_dispatch();

	/* closing */
	printf("\n");
	printf("shutdown httpd...\n");
	evhttp_free(httpd);

	return 0;
}
