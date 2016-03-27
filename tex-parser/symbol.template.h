#pragma once
#include "config.h"

enum symbol_id {
	/* nil symbol */
	S_NIL,

	/* index tree degree valve */
	S_DEGREE_VALVE = OPTR_INDEX_TREE_DEGREE_VALVE,

	/* special symbols */
	S_bignum,
	S_zero,
	S_one,
	S_float,

	/* auto-generated symbol set */
	/* INSERT_HERE */

	/* number of non-number and non-alphabet symbols */
	S_N
};
