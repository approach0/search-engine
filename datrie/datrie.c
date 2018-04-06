#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datrie.h"

#define DATRIE_MAX_STATE_TRANSFER UINT8_MAX
//#define DATRIE_DEBUG

enum walk_cntl {
	WALK_CNTL_REWALK,
	WALK_CNTL_CONTINUE,
	WALK_CNTL_BREAK
};

enum new_state_add_res {
	NEW_STATE_ALREADY_THERE,
	NEW_STATE_CONFLICT,
	NEW_STATE_ADDED
};

typedef enum walk_cntl
(*walk_cb_t)(struct datrie*, datrie_state_t, datrie_state_t, void*);

struct datrie datrie_new()
{
	struct datrie dat;
#ifdef DATRIE_DEBUG
	dat.len = 3;
#else
	dat.len = DATRIE_MAX_STATE_TRANSFER;
#endif
	dat.base = calloc(dat.len, sizeof(datrie_state_t));
	dat.check = calloc(dat.len, sizeof(datrie_state_t));
	dat.max_val = 0; /* indicates no value yet */

	/*
	 * It is just our convention here to put 'len' and 'max_val'
	 * into base[0] and check[0].
	 * base[0] and check[0] does not matter here, since
	 * our root is chosen to be state 1 initially. Here we
	 * avoid to choose state 0 as root because if later
	 * we try to find the children of root, but every check[i]
	 * is set to 0 initially, we then do not know which
	 * nodes are children of the root (assume it stays at
	 * state 0).
	 */
	dat.base[0] = dat.len;
	dat.check[0] = dat.max_val;

	dat.base[1] = 2 /* standard implementation uses 1 here */;
	/* we use 2 here because in our implementation a state
	 * transfer of delta 0 indicates the special tail/value node.
	 * And we do not want every time when a tail node is inserted,
	 * we need to solve conflict. (Because if it is 1, then 1 + 0
	 * is 1 which conflicts to the root here) */

	return dat;
}

static void datrie_realloc(struct datrie *dat, datrie_state_t new_len)
{
	datrie_state_t *tmp = NULL;

	tmp = realloc(dat->base, sizeof(datrie_state_t) * new_len);
	if (NULL == tmp) {
		fprintf(stderr, "realloc base[] failed.\n");
		free(dat->base);
		abort();
	} else {
		/* success, tmp is pointing to reallocated memory and original
		 * memory is deallocated. */
		dat->base = tmp;
	}

	tmp = realloc(dat->check, sizeof(datrie_state_t) * new_len);
	if (NULL == tmp) {
		fprintf(stderr, "realloc check[] failed.\n");
		free(dat->check);
		abort();
	} else {
		/* success, tmp is pointing to reallocated memory and original
		 * memory is deallocated. */
		dat->check = tmp;
	}

	/* fill the newly allocated area with zeros */
	datrie_state_t i, old_len = dat->len;
	for (i = old_len; i < new_len; i++) {
		dat->base[i] = 0;
		dat->check[i] = 0;
	}

	/* update length */
	dat->len = new_len;

#ifdef DATRIE_DEBUG
	printf("DATrie space re-allocating to new length %u ...\n", new_len);
	// datrie_print(*dat, !0 /* print all rows */);
#endif
}

void datrie_free(struct datrie dat)
{
	free(dat.base);
	free(dat.check);
}

void datrie_print(struct datrie dat, int print_all)
{
	printf("DATrie (len=%u, max_val=%u)\n", dat.len, dat.max_val);
	printf("%6s", "state");
	printf("%6s", "base");
	printf("%6s", "check");
	printf("\n");

	for (datrie_state_t i = 0; i < dat.len; i++) {
		/*
		 * we should not rely on check[i] to decide
		 * if it is dirty, otherwise we will not see
		 * the root node since it has a zero check value.
		 */
		if (print_all || dat.base[i] != 0) {
			printf("[%3d]", i);
			printf(" %3d ", dat.base[i]);
			printf(" %3d ", dat.check[i]);
			printf("\n");
		}
	}
}

datrie_state_t datrie_cmap(const char c)
{
	/*
	 * TODO
	 * you can define more practical map function here,
	 * to achieve better space efficiency, for example,
	 * map to datrie_state_t based on frequency.
	 */

	/* convert signed char to unsigned char, to avoid the
	 * effect of "sign extension" */
	uint8_t uc = (uint8_t)c;

	if (uc == '\0')
		/* at the end, we may want to return an zero value
		 * to indicate this is the finishing walk step. */
		return 0;
	else
		return (datrie_state_t)(uc - 'a' + 1);
}

static datrie_state_t
datrie_walk(struct datrie *dat, const char *utf8_str, walk_cb_t cb, void *arg)
{
	datrie_state_t next_state, cur_state = 1;
	datrie_state_t c /* state transfer */, str_idx = 0;

	if (strlen(utf8_str) == 0)
		return 0;

rewalk:
	while (str_idx <= strlen(utf8_str)) {
		c = datrie_cmap(utf8_str[str_idx]);
		next_state = dat->base[cur_state] + c; /* state transfer */

		enum walk_cntl walk_cntl = cb(dat, cur_state, next_state, arg);
		if (WALK_CNTL_REWALK == walk_cntl) {
			cur_state = 1;
			str_idx = 0;
#ifdef DATRIE_DEBUG
			printf("Re-walk ...");
#endif
			goto rewalk;
		} else if (WALK_CNTL_BREAK == walk_cntl) {
			return 1;
		}

		cur_state = next_state;
		str_idx ++;
	}

	return 0;
}

static datrie_state_t *
datrie_children(struct datrie *dat, datrie_state_t state)
{
	datrie_state_t *child = malloc(
		sizeof(datrie_state_t) * (DATRIE_MAX_STATE_TRANSFER + 1 /*'\0'*/)
	);

	datrie_state_t i, j = 0;
	for (i = 1; i < dat->len; i++) {
		if (dat->check[i] == state)
			child[j++] = i;
	}
	child[j] = 0; /* terminal sign */

	return child;
}

static datrie_state_t
shift_alloc(struct datrie *dat, datrie_state_t *child,
           datrie_state_t conflict_state)
{
	datrie_state_t j, max_new_state = 0, try_shift = 1;
	do {
		try_shift ++;
		for (j = 0; child[j] != 0; j++) {
			datrie_state_t child_new_state = try_shift + child[j];

			/* keep watching the maximum possible new state value */
			if (child_new_state > max_new_state)
				max_new_state = child_new_state;

			/* for children shifted out of boundary, it is not conflict. */
			if (child_new_state >= dat->len)
				continue;

			/* make sure this new state does not conflict with other
			 * existing state AND it does not conflict the conflict_state
			 * which is inserting now. */
			if (child_new_state == conflict_state ||
			    dat->base[child_new_state] != 0)
				break; /* no luck, try another */
		}
		if (child[j] == 0)
			break; /* all children can accept this, we will be good. */
	} while (1);

	/* allocate memory if necessary */
	if (max_new_state >= dat->len)
		datrie_realloc(dat, max_new_state + 1); /* most lazy policy */

	return try_shift;
}

static void
relocate(struct datrie *dat, datrie_state_t state, datrie_state_t *child,
         datrie_state_t new_base)
{
	datrie_state_t *grand_child, j, k;
	for (j = 0; child[j] != 0; j++) {
		/* value parenthesis is shift value, it must be greater than zero */
		datrie_state_t new_state = (new_base - dat->base[state]) + child[j];
#ifdef DATRIE_DEBUG
		printf("Move child %u of %u to new state %u.\n",
		       child[j], state, new_state);
#endif
		/* copy child base value to new state */
		dat->base[new_state] = dat->base[child[j]];
		dat->check[new_state] = state; /* set child parent */
		/* erase old state child */
		dat->base[child[j]] = 0;
		dat->check[child[j]] = 0;

		/* We also have to update grand children CHECK values */
		grand_child = datrie_children(dat, child[j]);

		for (k = 0; grand_child[k] != 0; k++) {
#ifdef DATRIE_DEBUG
			printf("Update %u's child CHECK[%u] value to %u \n",
			       child[j], grand_child[k], new_state);
#endif
			dat->check[grand_child[k]] = new_state;
		}
		free(grand_child);
	}

#ifdef DATRIE_DEBUG
	printf("Finally set the new base BASE[%d] to %d \n", state, new_base);
#endif
	dat->base[state] = new_base;
}

static void
resolve(struct datrie *dat,
        datrie_state_t state, datrie_state_t conflict_state)
{
	datrie_state_t new_base, *child = datrie_children(dat, state);
	new_base = dat->base[state] + shift_alloc(dat, child, conflict_state);
#ifdef DATRIE_DEBUG
	printf("Conflict between one child of state %d and %d \n",
	       state, conflict_state);
#endif
	/* relocate all children of "state" to the new position determined
	 * by new_base */
	relocate(dat, state, child, new_base);
	free(child);
}

static enum new_state_add_res
attach_new_state(struct datrie *dat, datrie_state_t cur,
                 datrie_state_t new, datrie_state_t val)
{
	if (dat->base[new] == 0) {
		/* space is available */
		dat->base[new] = val;
		dat->check[new] = cur;
		return NEW_STATE_ADDED;

	} else if (dat->check[new] != cur) {
		resolve(dat, dat->check[new], new);
		return NEW_STATE_CONFLICT;

	} else {
		return NEW_STATE_ALREADY_THERE;
	}
}

static enum walk_cntl
insert_walk_cb(struct datrie *dat, datrie_state_t cur, datrie_state_t next,
               void *_ /* not used */)
{
	datrie_state_t new_val = next + 1; /* make one room for tail node */

	if (next >= dat->len)
		datrie_realloc(dat, next + 1); /* most lazy policy */

	if (next == dat->base[cur] + 0) /* tail node */
		new_val = dat->max_val + 1;

	enum new_state_add_res res = attach_new_state(dat, cur, next, new_val);
	if (NEW_STATE_CONFLICT == res) {
		/* there is conflict, although resolved,
		 * we have to restart from string head
		 * in case any ancestor state is changed. */
		return WALK_CNTL_REWALK;
	} else if (NEW_STATE_ADDED == res /* newly added */ &&
	           next == dat->base[cur] + 0 /* tail node */) {
		/* (just to ensure we only update max_val once) */
		dat->max_val += 1;
	}

	return WALK_CNTL_CONTINUE;
}

datrie_state_t datrie_insert(struct datrie *dat, const char *utf8_str)
{
	datrie_state_t save = dat->max_val;

	/* datrie_walk in insertion case will always return 0 */
	(void)datrie_walk(dat, utf8_str, &insert_walk_cb, NULL);

	/* check if we indeed inserted a new value */
	if (dat->max_val == save)
		return 0;
	else
		return dat->max_val;
}

static enum walk_cntl
lookup_walk_cb(struct datrie *dat, datrie_state_t cur, datrie_state_t next,
               void *arg)
{
	datrie_state_t *found = (datrie_state_t *)arg;

	if (dat->check[next] != cur /* non-existing node */ ||
	    next >= dat->len        /* out of index */)
		return WALK_CNTL_BREAK; /* give up searching */

	if (next == dat->base[cur] + 0 /* tail node */)
		*found = dat->base[next]; /* found */

	return WALK_CNTL_CONTINUE;
}

datrie_state_t datrie_lookup(struct datrie *dat, const char *utf8_str)
{
	datrie_state_t found = 0;
	if (0 == datrie_walk(dat, utf8_str, &lookup_walk_cb, &found)) {
		return found;
	} else {
		/* out of index */
		return 0;
	}
}
