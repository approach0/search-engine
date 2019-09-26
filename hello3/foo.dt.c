#require <stdio.h>
#require "linkli/list.h"
#require "strmap/strmap.h"
#require "sds/sds.h"

void list_demo()
{
	struct person {
		char name[128];
		int age, salary;
	};

	struct person a[] = {{"Tim", 23, 200}, {"Denny", 35, 982}, {"Wei", 28, 0}};

	linkli_t l = NULL;
	li_append_array(l, a);

	printf("  BEFORE DELETION (size = %lu) \n", li_size(l));
	foreach (iter, li, l) {
		struct person *a = li_element(a, iter->cur);
		printf("name: %s, age: %d, salary: %d\n", a->name, a->age, a->salary);
	}

	foreach (iter, li, l) {
		struct person *a = li_element(a, iter->cur);
		if (a->name[0] == 'W') {
			*iter = li_remove(&l, iter->cur);
			printf("free %s\n", a->name);
			free(a);
			break;
		}
	}

	printf("  AFTER DELETION (size = %lu) \n", li_size(l));
	foreach (iter, li, l) {
		struct person *a = li_element(a, iter->cur);
		printf("name: %s, age: %d, salary: %d\n", a->name, a->age, a->salary);
	}

	foreach (iter, li, l) {
		struct person *a = li_element(a, iter->cur);
		*iter = li_remove(&l, iter->cur);

		printf("free %s\n", a->name);
		free(a);
	}

	printf("list empty? %d\n", li_empty(l));
}

void dict_demo()
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

void string_demo()
{
	sds s = sdsnew("Hello");
	s = sdscat(s, " ");
	s = sdscat(s, "World");
	printf("%s\n", s);

	int cnt;
	sds *tokens = sdssplitlen(s, sdslen(s), " ", 1, &cnt);
	for (int i = 0; i < cnt; i++) {
		printf("%s\n", tokens[i]);
	}

	for (int i = 0; i < cnt; i++)
		sdsfree(tokens[i]);
	free(tokens);
	sdsfree(s);
}
