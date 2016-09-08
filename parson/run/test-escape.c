#include <stdio.h>

#include "mhook/mhook.h"
#include "parson.h"

void test_string_encode()
{
	char *enc_str;
	char str[1024];
	int i;

	for (i = 0x01; i < 0x30; i++)
		str[i - 1] = i;
	str[i - 1] = '\0';

	enc_str = json_encode_string(str);
	printf("encoded string: %s\n", enc_str);
	free(enc_str);
}

int main()
{
	test_string_encode();
	mhook_print_unfree();
	return 0;
}
