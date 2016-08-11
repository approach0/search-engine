/* httpd on receive callback */
typedef const char *(*httpd_on_recv_cb)(const char*, void*);

/* httpd start and loop function */
int httpd_run(unsigned short, httpd_on_recv_cb, void*);
