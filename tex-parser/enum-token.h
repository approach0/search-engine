enum token_id {
	/* nil token */
	T_NIL,

	/* token with commutative law */
	T_ADD,
	T_TIMES,
	T_TAB,
	T_SEP_CLASS,
	T_HANGER,

	/* sequential token */
	T_CHOOSE,
	T_SQRT,
	T_FRAC,
	T_MODULAR,

	/* core tokens */
	T_VAR,
	T_NUM,
	T_ONE,
	T_ZERO,
	T_SUM_CLASS,
	T_FUN_CLASS,
	T_EQ_CLASS,
	T_FLOAT,
	T_STAR,
	T_POS,
	T_NEG,
	T_VERT,
	T_FACT,
	T_PRIME,
	T_SUB_SCRIPT,
	T_SUP_SCRIPT,
	T_DOTS,
	T_PARTIAL,
	T_PI,
	T_INFTY, 
	T_EMPTY, 
	T_ANGLE, 
	T_PERP, 
	T_CIRC, 
	T_PERCENT, 
	T_VECT,
	T_CEIL,
	T_FLOOR,

	T_X_ARROW,

	/* number of tokens */
	T_N
};
