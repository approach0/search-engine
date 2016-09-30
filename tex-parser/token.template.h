enum token_id {
	/* nil token */
	T_NIL,

	/* index tree degree valve */
	T_DEGREE_VALVE = OPTR_INDEX_TREE_DEGREE_VALVE,
	
	/* index tree rank valve */
	T_MAX_RANK = OPTR_INDEX_TREE_DEGREE_VALVE + OPTR_INDEX_RANK_MAX,

	/* special tokens */
	T_ZERO,
	T_ONE,
	T_NUM,
	T_FLOAT,
	T_HANGER,
	T_BASE,

	/* pair tokens */
	T_GROUP,
	T_CEIL,
	T_FLOOR,
	T_VERTS,

	/* auto-generated token set */
	/* INSERT_HERE */

	/* number of tokens */
	T_N
};
