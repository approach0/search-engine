#include "mhook/mhook.h"
#include "yajl/yajl_gen.h"

#include <stdio.h>
#include <string.h>

int main()
{
	const char str[] = "/\"hello 世界\"\\";
	printf("original string: %s\n", str);
	yajl_gen gen;
	yajl_gen_status st;

	gen = yajl_gen_alloc(NULL);
	yajl_gen_config(gen, yajl_gen_validate_utf8, 1);
	//yajl_gen_config(gen, yajl_gen_escape_solidus, 1);
	st = yajl_gen_string(gen, (const unsigned char*)str, strlen(str));

	if (yajl_gen_status_ok == st) {
		const unsigned char * buf;
		size_t len;
		yajl_gen_get_buf(gen, &buf, &len);

		printf("encoded string: ");
		fwrite(buf, 1, len, stdout);
		printf("\n");
	}

	yajl_gen_free(gen);

	mhook_print_unfree();
	return 0;
}
