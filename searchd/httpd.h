typedef char *(*httpd_on_recv_cb)(const char*, void*);

int
httpd_run(unsigned short, httpd_on_recv_cb, void*);
