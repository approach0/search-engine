#pragma once
#include "config.h"

enum token_id {
	/* nil token */
	T_NIL,

	/* index tree degree valve */
	T_DEGREE_VALVE = OPTR_INDEX_TREE_DEGREE_VALVE,

	/* special tokens */
	T_ZERO,
	T_ONE,
	T_NUM,
	T_FLOAT,

	/* auto-generated token set */
	/* INSERT_HERE */

	/* number of tokens */
	T_N
};
