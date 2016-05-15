#pragma once
#include <stddef.h> /* NULL */
#include <stdint.h> 
#include <stdbool.h>

/*
 * a typical doubly-linked list node
 */
struct list_node {
	struct list_node *prev, *next;
};

/*
 * list node constructor (circular list)
 */
#define LIST_NODE_CONS(_ln) \
	(_ln).prev = &(_ln); \
	(_ln).next = &(_ln)

/* 
 * list iterator 
 *
 * One of the reasons that my list iterator is designed this way 
 * is that the advantage to manipulate list as a string (e.g. 
 * insert a string of list between two nodes) instead of just 
 * one node.
 */
struct list_it {
	struct list_node *now, *last;
};

/* for better code understanding, we alias list iterator when
 * the iterator is pointed to the head/tail of a list.
 */
typedef struct list_it list;

#define LIST_NULL {NULL, NULL}

#define LIST_CONS(_it) \
	(_it).now = NULL; \
	(_it).last = NULL

/*
 * get a iterator pointing to a list node.
 */
static __inline struct list_it
list_get_it(struct list_node *in_node)
{
	struct list_it res;

	if (in_node == NULL)
		res.now = res.last = NULL;
	else {
		res.now = in_node;
		res.last = in_node->prev;
	}

	return res;
}

/*
 * this is the core basic operation of our doubly linked
 * circular list, to fasten and unfasten two strings of 
 * list pointed by two list iterators respectively.
 *
 * After this operation, list node i->now will still point to
 * what it used to point.
 */
static __inline void
list_tk(struct list_it *i0, struct list_it *i1)
{
	if (i0->now == NULL)
		*i0 = *i1;
	else if (i1->now == NULL)
		*i1 = *i0;

	i0->now->prev = i1->last;
	i0->last->next = i1->now;

	i1->now->prev = i0->last;
	i1->last->next = i0->now;

	i0->last = i1->last;
	i1->last = i1->now->prev;
}

/*
 * list_foreach go through a list and call function passed by
 * 'each_do' for each list node, until 'each_do' function return
 * LIST_RET_BREAK.
 *
 * note: 'fwd' is short for 'forward' which points to the next 
 * iteration nodes.
 */
typedef bool list_it_fun(struct list_it*, 
		struct list_it*, 
		struct list_it*,
		void*);

#define LIST_RET_BREAK    1
#define LIST_RET_CONTINUE 0

static __inline struct list_it
list_foreach(struct list_it *head, list_it_fun *each_do, 
		void *extra) 
{
	struct list_it fwd, now = *head;

	if (head->now == NULL)
		return now;

	for(;;) {
		fwd = list_get_it(now.now->next);
		if (LIST_RET_BREAK == 
				each_do(head, &now, &fwd, extra))
			break;

		now = fwd;
	}

	return now;
}

/*
 * the following functions do what their name tells you.
 * Generally you just pass NULL to the last two arguments,
 * however, you MUST pass 'now' and 'fwd' if you are using
 * them during the list node iteration (called by list_foreach)
 * for safety reason.
 *
 * list_detach_one returns whether the detached node is the last
 * node in list.
 */
static __inline void
list_insert_one_at_tail(struct list_node *node, 
		struct list_it *head, 
		struct list_it *now,
		struct list_it *fwd)
{
	struct list_it tmp = list_get_it(node);
	list_tk(head, &tmp);

	if (now) {
		*now = list_get_it(now->now);
		*fwd = list_get_it(now->now->next);
	}
}

static __inline void
list_insert_one_at_head(struct list_node *node, 
		struct list_it *head, 
		struct list_it *now,
		struct list_it *fwd)
{
	list_insert_one_at_tail(node, head, now, fwd);
	*head = list_get_it(head->last);
}

static __inline bool
list_detach_one(struct list_node *node,
		struct list_it *head, 
		struct list_it *now,
		struct list_it *fwd)
{
	struct list_it inow = list_get_it(node);
	struct list_it inew = list_get_it(node->next);

	if (head->now == head->last)
		*head = list_get_it(NULL);
	else {
		list_tk(&inow, &inew);
		
		if (head->now == node)
			/* removed is the head */
			*head = list_get_it(inew.now);

		*head = list_get_it(head->now);
	}

	if (now) {
		if (now->now == node) {
			/* go back one step so that next iterator in
			 * list_foreach will go back to 'inew.now'.*/
			*now = list_get_it(inew.now->prev);
		}
		*now = list_get_it(now->now);
		*fwd = list_get_it(now->now->next);
	}

	if (head->now == NULL)
		return LIST_RET_BREAK;
	else
		return LIST_RET_CONTINUE;
}

/* 
 * these macros help you define a list_foreach callback 
 * function & return the right value if you want to go over 
 * the list.
 */
#define LIST_IT_CALLBK(_fun_name) \
	bool _fun_name(struct list_it *pa_head, \
			struct list_it *pa_now, \
			struct list_it *pa_fwd, \
			void *pa_extra)

#define LIST_GO_OVER \
	if (pa_now->now == pa_head->last) \
		return LIST_RET_BREAK; \
	else \
		return LIST_RET_CONTINUE

/*
 * these macros provide a common way to access a struct giving
 * its member addresss. Specifically, use LIST_OBJ to get the 
 * list object by passing the member name of a list node.
 */
#define MEMBER_OFFSET(_type, _member) \
	((uintptr_t)&((_type*)0)->_member)

#define MEMBER_2_STRUCT(_member_addr, _type, _member_name) \
	(_member_addr == NULL) ? NULL : \
	(_type*)((uintptr_t)(_member_addr) - MEMBER_OFFSET(_type, _member_name))

#define LIST_OBJ(_type, _name, _list_node_name) \
	_type* _name = MEMBER_2_STRUCT(pa_now->now, _type, _list_node_name)

/*
 * this macro utility helps you with pointer type cast.
 */
#define P_CAST(_to, _type, _from) \
	_type* _to = (_type*)(_from)

/*
 * comparator function, called specifically by list_sort.
 */
typedef bool list_cmp_fun(struct list_node *, 
		struct list_node *,
		void *);

struct list_sort_arg {
	list_cmp_fun     *cmp;
	void             *extra;
};

struct __list_sort_tmp_arg { 
	struct list_it   it;
	void             *extra;
};

#define LIST_CMP_CALLBK(_fun_name) \
	bool _fun_name(struct list_node *pa_node0, \
			struct list_node *pa_node1, \
			void *pa_extra)
/*
 * list insert is designed as a callback called by list_foreach
 * function, it can also be used by user-end to insert a node into 
 * certain position.
 * see list_sort on how to use.
 */
#define __CMP(_node) \
	(*sort->cmp)(expa->it.now, pa_now->_node, expa->extra)

#include <stdio.h>
static __inline LIST_IT_CALLBK(list_insert)
{
	P_CAST(sort, struct list_sort_arg, pa_extra);
	P_CAST(expa, struct __list_sort_tmp_arg, sort->extra);

	if (pa_now->now == pa_head->now) {
		if(__CMP(now)) {
			list_tk(&expa->it, pa_head);
			*pa_head = list_get_it(pa_head->last);
			return LIST_RET_BREAK;
		} else if (!__CMP(last)) {
			list_tk(&expa->it, pa_head);
			return LIST_RET_BREAK;
		}
	} else if (__CMP(now)) {
		list_tk(&expa->it, pa_now);
		return LIST_RET_BREAK;
	} 

	return LIST_RET_CONTINUE;
}

static __inline void
list_sort_insert(struct list_node *ln, struct list_it *head,
                 struct list_sort_arg *sort)
{
	struct __list_sort_tmp_arg expa;
	expa.it = list_get_it(ln);
	expa.extra = sort->extra; /* save */
	sort->extra = &expa;

	if (head->now)
		list_foreach(head, &list_insert, sort);
	else
		list_insert_one_at_tail(ln, head, NULL, NULL);
	
	sort->extra = expa.extra; /* restore */
}

/*
 * list sort
 * struct list_cmp_arg is used as argument list here.
 * where 'cmp' is sorting compare callback;
 * 'extra' is your extra parameters you need to pass to your 
 * compare callback.
 */
static __inline void
list_sort(struct list_it *head, struct list_sort_arg *sort)
{
	struct list_it tmp_hd = {NULL, NULL};
	struct list_node *node;
	struct __list_sort_tmp_arg expa;
	bool res = 0;

	expa.extra = sort->extra; /* save */
	sort->extra = &expa;
	
	while (!res) {
		node = head->now;
		res = list_detach_one(node, head, NULL, NULL);
		expa.it = list_get_it(node);

		if (tmp_hd.now)
			list_foreach(&tmp_hd, &list_insert, sort);
		else
			list_tk(&expa.it, &tmp_hd);
	}

	*head = tmp_hd;
	sort->extra = expa.extra; /* restore */
}

/*
 * this macro provides a quick way to define a list-release 
 * function for convenience.
 */
#define LIST_DECL_FREE_FUN(_fun_name) \
	extern void _fun_name(struct list_it *)
	
#define LIST_DEF_FREE_FUN(_fun_name, _type, _member, _statm) \
	static LIST_IT_CALLBK(_ ## _fun_name) \
	{ \
		LIST_OBJ(_type, p, _member); \
		bool res = list_detach_one(pa_now->now, \
		               pa_head, pa_now, pa_fwd); \
		_statm; /* free(p); */ \
		return res; \
	} \
	void _fun_name(struct list_it *head) \
	{ \
		list_foreach(head, &_ ## _fun_name, NULL); \
	} LIST_DECL_FREE_FUN(_fun_name)
