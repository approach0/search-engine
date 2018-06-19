#include "common/common.h"
#include "mhook/mhook.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist.h"
#include "math-postlist.h"

int main()
{
	struct math_postlist_item *items;
	struct postlist *po;

	po = math_postlist_create_plain();
	PTR_CAST(codec, struct postlist_codec, po->buf_arg);
	items = postlist_random(20, codec->fields);

	postlist_print(items, 20, codec->fields);

	postlist_free(po);
	free(items);

	mhook_print_unfree();
	return 0;
}
