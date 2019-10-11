#define prerr(_fmt, ...) \
	fprintf(stderr, C_RED "Error: " C_RST _fmt "\n", ##__VA_ARGS__)

#define prinfo(_fmt, ...) \
	fprintf(stderr, C_BLUE "Info: " C_RST _fmt "\n", ##__VA_ARGS__)

#define WHERE fprintf(stderr,"[WHERE]%s: %d\n", __FILE__, __LINE__)

#define STRVAR_PAIR(_var) \
	#_var, _var

#include <stddef.h>
#include <stdio.h>
static __inline__ void prbuff_bytes(void *_buf, size_t sz)
{
	unsigned char *buf = (unsigned char*)(_buf);
	fprintf(stderr, "debug: sz=%lu \n", sz);
	for (int i = 0; i < sz; i++)
		fprintf(stderr, "%x", 0xff & buf[i]);
	fprintf(stderr, "\n");
}

static __inline__ void prbuff_uints(unsigned int *ints, size_t n)
{
	fprintf(stderr, "debug: n=%u \n", n);
	for (int i = 0; i < n; i++)
		fprintf(stderr, "%u ", ints[i]);
	fprintf(stderr, "\n");
}

static __inline__ void prbuff_uints16(unsigned short *ints, size_t n)
{
	fprintf(stderr, "debug: n=%u \n", n);
	for (int i = 0; i < n; i++)
		fprintf(stderr, "%u ", ints[i]);
	fprintf(stderr, "\n");
}
