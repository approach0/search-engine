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

static void log_client_IP(struct evhttp_request *req, FILE *fh)
{
	char *ip;
	unsigned short port;
	struct evhttp_connection *evcon;
	evcon = evhttp_request_get_connection(req);

	if (evcon) {
		evhttp_connection_get_peer(evcon, &ip, &port);
		/* no need to free string ip at this point,
		 * it will be released when req is freed. */

		fprintf(fh, "%s", ip);
		fflush(fh);
	} else {
		fprintf(fh, "evhttp_connection_get_peer() "
		        "fails.\n");
	}
}

char *get_POST_str(struct evhttp_request *req)
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

#ifdef DEBUG_PRINT_HTTP_HEAD
	printf("HTTP request header:\n");
	print_headers(req);
#endif

#ifdef SEARCHD_LOG_ENABLE
#ifdef SEARCHD_LOG_CLIENT_IP
	FILE *log_fh = fopen(SEARCHD_LOG_FILE, "a");

	if (log_fh == NULL) {
		fprintf(stderr, "cannot open %s.\n", SEARCHD_LOG_FILE);
	} else {
		fprintf(log_fh, "from IP: ");
		log_client_IP(req, log_fh);
		fprintf(log_fh, "\n");
		fclose(log_fh);
	}
#endif
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
	if (response)
		evbuffer_add_printf(buf, "%s", response);

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

	/* check if port is already binded */
	if (httpd == NULL) {
		fprintf(stderr, "Failed to listen on port %d.\n", port);
		return 1;
	}

	/* set callback functions on receving */
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
