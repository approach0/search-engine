#define prerr(_fmt, ...) \
	fprintf(stderr, C_RED "Error: " C_RST _fmt "\n", ##__VA_ARGS__);

#define prinfo(_fmt, ...) \
	fprintf(stderr, C_BLUE "Info: " C_RST _fmt "\n", ##__VA_ARGS__);

#define WHERE fprintf(stderr,"[WHERE]%s: %d\n", __FILE__, __LINE__);

#define strvar_pair(_var) \
	#_var, _var

#include <stddef.h>
#include <stdio.h>
static __inline__ void prbuff(void *_buf, size_t sz)
{
	unsigned char *buf = (unsigned char*)(_buf);
	fprintf(stderr, "debug: sz=%lu \n", sz);
	for (int i = 0; i < sz; i++)
		fprintf(stderr, "%x", 0xff & buf[i]);
	fprintf(stderr, "\n");
}
