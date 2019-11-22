#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "strmap.h"

void test_1()
{
	struct strmap *m = strmap_new();

	(*strmap_val_ptr(m, "C")) = "programming";
	(*strmap_val_ptr(m, "hello")) = "world";
	(*strmap_val_ptr(m, "foo")) = "bar";
	(*strmap_val_ptr(m, "string")) = "map";
	(*strmap_val_ptr(m, "world")) = "cup";
	(*strmap_val_ptr(m, "lab")) = "meeting";
	(*strmap_val_ptr(m, "math")) = "search";
	(*strmap_val_ptr(m, "pokemon")) = "go";
	(*strmap_val_ptr(m, "publish")) = "paper";
	(*strmap_val_ptr(m, "core")) = "dumped";

	printf("%s\n", (char *)(*strmap_val_ptr(m, "world")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "string")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "hello")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "math")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "foo")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "C")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "pokemon")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "core")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "lab")));

	strmap_free(m);
}

struct strmap *test_2()
{
	struct strmap *m = strmap_new();

	(*strmap_val_ptr(m, "name")) = strdup("Tim");
	printf("slots: %u, len: %u\n", m->n_entries, m->length);
	(*strmap_val_ptr(m, "age")) = strdup("23");
	printf("slots: %u, len: %u\n", m->n_entries, m->length);
	(*strmap_val_ptr(m, "school")) = strdup("rit");
	printf("slots: %u, len: %u\n", m->n_entries, m->length);

	return m;
}

void test_3()
{
	struct strmap *d = strmap(
		"good", "bad",
		"small", "large",
		"pretty", "ugly"
	);

	d["new"] = "old";
	d["cool"] = "hot";
	d["hot"] = "cool";

	printf("d[%s] = %s\n", "hot", (char*)d["hot"]);

	foreach (iter, strmap, d) {
		char *key = iter->cur->keystr;
		printf("d[%s] = %s\n", key, (char*)d[[key]]);
	}

	printf("dict empty? %d\n", strmap_empty(d));
	strmap_free(d);
}


void test_valist_construct()
{
	struct strmap *m = strmap(
		"good", "bad",
		"small", "large",
		"pretty", "ugly"
	);

	printf("%s\n", (char *)(*strmap_val_ptr(m, "good")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "small")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "pretty")));
	printf("\n");

	strmap_free(m);
}

void test_iterator()
{
	strmap_t m = NULL;
	printf("strmap empty? %d\n", strmap_empty(m));

	m = strmap(
		"good", "bad",
		"small", "large",
		"pretty", "ugly"
	);
	printf("strmap empty? %d\n", strmap_empty(m));
	
	foreach (iter, strmap, m) {
		printf( "%s -> %s\n", iter->cur->keystr, (char*)iter->cur->value);
	}
	
	strmap_free(m);
}

int main()
{
	printf("test_1\n");
	test_1();
	printf("\n");

	printf("test_2\n");
	struct strmap *m = test_2();
	printf("\n");

	printf("test_3\n");
	test_3();
	printf("\n");

	printf("%s\n", (char *)(*strmap_val_ptr(m, "name")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "age")));
	printf("%s\n", (char *)(*strmap_val_ptr(m, "school")));

	free(*strmap_val_ptr(m, "name"));
	free(*strmap_val_ptr(m, "age"));
	free(*strmap_val_ptr(m, "school"));
	strmap_free(m);
	printf("\n");

	printf("test_valist_construct\n");
	test_valist_construct();
	printf("\n");

	printf("test_iterator\n");
	test_iterator();
	printf("\n");

	mhook_print_unfree();
	return 0;
}
