#include <stdio.h>

#include "mhook/mhook.h"
#include "parson.h"


void test_string_encode()
{
	char *enc_str;
	const char str[] = "/\"hello 世界\"\\";

	printf("original string: %s\n", str);

	enc_str = json_encode_string(str);
	printf("encoded string: %s\n", enc_str);
	free(enc_str);
}

void test_simple_decode()
{
	const char json[] =
	"{"
		"\"name\" : \"可口可乐 \\\"Diet\\\"\""
	"}";

	JSON_Value *val = json_parse_string(json);
    JSON_Object *obj = json_value_get_object(val);
	const char *str = json_object_get_string(obj, "name");

	printf("%s \n", str);

	json_value_free(val);
}

void test_array_decode()
{
	const char json[] =
	"{"
		"\"page\" : 1, "
		"\"kw\" : ["
			"{\"type\" : \"tex\", \"str\" : \"f(x)\"}, "
			"{\"type\" : \"term\", \"str\" : \"such that\"}"
		"]"
	"}";

	double num;
	size_t i;
	JSON_Array *arr;

	JSON_Value *val = json_parse_string(json);
    JSON_Object *obj = json_value_get_object(val);

	printf("original JSON:\n%s\n", json);

	num = json_object_get_number(obj, "page");
	printf("page: %u.\n", (unsigned int)num);

	arr = json_object_get_array(obj, "kw");

    if (arr == NULL) {
		fprintf(stderr, "cannot get JSON array!\n");
		return;
	}

	for (i = 0; i < json_array_get_count(arr); i++) {
		JSON_Object *sub_obj = json_array_get_object(arr, i);
		const char *str;

		str = json_object_get_string(sub_obj, "type");
		printf("type: %s\n", str);

		str = json_object_get_string(sub_obj, "str");
		printf("str: %s\n", str);
	}

	json_value_free(val);
}

int main()
{
	test_simple_decode();
	test_array_decode();
	test_string_encode();

	mhook_print_unfree();
	return 0;
}
