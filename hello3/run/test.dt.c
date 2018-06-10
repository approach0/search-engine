#require "mhook/mhook.h"
#require "foo.h"

int main()
{
	string_demo();
	list_demo();
	dict_demo();

	mhook_print_unfree();
	return 0;
}
