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

	foreach(iter, list, li) {
		struct person *a = list_element(a, iter.cur);
		printf("name: %s, age: %d, salary: %d\n", a->name, a->age, a->salary);
	}

	foreach(iter, list, li) {
		int sz = list_size(iter.cur);
		list_detach(iter.cur->prev, iter.cur->next);

		struct person *a = list_element(a, iter.cur);
		printf("free %s\n", a->name);
		free(a);

		if (sz - 1 == 0) {
			li = NULL;
			break;
		}
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

	foreach (iter, strmap, d) {
		char *key = iter.cur->keystr;
		printf("d[%s] = %s\n", "pretty", d["pretty"]);
	}

	strmap_free(d);
}

int main()
{
	list_demo();
	dict_demo();

	return 0;
}
