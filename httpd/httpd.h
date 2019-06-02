/* httpd on receive callback */
typedef const char *(*httpd_on_recv_cb)(const char*, void*);

struct uri_handler {
	char             uri[1024];
	httpd_on_recv_cb handler;
};

/* httpd start and loop function */
int httpd_run(unsigned short, struct uri_handler*, unsigned int, void*);
