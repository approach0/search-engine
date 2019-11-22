#require <stdio.h>
#require "mhook/mhook.h"
#require "linkli/list.h"

void test_list()
{
	struct person {
		char name[128];
		int age, salary;
	};

	struct person a[] = {
		{"Tim", 23, 200},
		{"Huk", 30, 9900},
		{"Denny", 35, 982},
		{"Wei", 28, 0}
	};

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
			li_iter_remove(iter, &l);
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
		unsigned int rounds = li_iter_remove(iter, &l);

		printf("free %s, left rounds: %u\n", a->name, rounds);
		free(a);
	}

	printf("list empty? %d\n", li_empty(l));
}

int main()
{
	test_list();
	mhook_print_unfree();
	return 0;
}
