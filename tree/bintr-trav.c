#include "bintr.h"

#define GO(_recur) \
	if (ref->this_->son[pos]) { \
		struct bintr_ref new_ref = \
		  {ref->this_, ref->this_->son + pos, ref->this_->son[pos]}; \
		_recur(&new_ref, level + 1, fun, arg); \
	} do {} while(0)

#define GO_RIGHT(_recur) \
	pos = BINTR_RIGHT; \
	GO(_recur)

#define GO_LEFT(_recur) \
	pos = BINTR_LEFT; \
	GO(_recur)

void
bintr_preorder(struct bintr_ref *ref, uint32_t level,
               bintr_it_callbk fun, void *arg)
{
	enum bintr_son_pos pos;

	if (fun(ref, level, arg) == BINTR_IT_STOP)
		return;

	GO_RIGHT(bintr_preorder);

	GO_LEFT(bintr_preorder);
}

void
bintr_inorder(struct bintr_ref *ref, uint32_t level,
              bintr_it_callbk fun, void *arg)
{
	enum bintr_son_pos pos;

	GO_LEFT(bintr_inorder);

	if (fun(ref, level, arg) == BINTR_IT_STOP)
		return;

	GO_RIGHT(bintr_inorder);
}

void
bintr_inorder_desc(struct bintr_ref *ref, uint32_t level,
                   bintr_it_callbk fun, void *arg)
{
	enum bintr_son_pos pos;

	GO_RIGHT(bintr_inorder_desc);

	if (fun(ref, level, arg) == BINTR_IT_STOP)
		return;

	GO_LEFT(bintr_inorder_desc);
}

void
bintr_postorder(struct bintr_ref *ref, uint32_t level,
               bintr_it_callbk fun, void *arg)
{
	enum bintr_son_pos pos;

	GO_LEFT(bintr_postorder);

	GO_RIGHT(bintr_postorder);

	if (fun(ref, level, arg) == BINTR_IT_STOP)
		return;
}
