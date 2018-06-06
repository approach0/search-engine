#require <stdio.h> // for printf
#require "list.h"
#require "strmap/strmap.h"

void list_demo()
{
	struct person {
		char name[128];
		int age, salary;
	};

	struct person a[] = {{"Tim", 23, 200}, {"Denny", 35, 982}, {"Wei", 28, 0}};

	list_t li = NULL;
	list_append_array(li, a);

	foreach (iter, list, li) {
		struct person *a = list_element(a, iter.cur);
		printf("name: %s, age: %d, salary: %d\n", a->name, a->age, a->salary);
	}

	foreach (iter, list, li) {
		struct person *a = list_element(a, iter.cur);
		printf("free %s\n", a->name);
		list_free_entry(struct person, iter);
	}

	printf("list empty? %d\n", list_empty(li));
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
		char *key = iter.cur->keystr;
		printf("d[%s] = %s\n", key, (char*)d[[key]]);
	}

	printf("dict empty? %d\n", strmap_empty(d));
	strmap_free(d);
}

int main()
{
	list_demo();
	dict_demo();

	return 0;
}
