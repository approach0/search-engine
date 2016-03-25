enum symbol_id {
	/* nil symbol */
	S_NIL,

	/* core symbols */
	S_ADD,
	S_TIMES,
	S_TAB,
	S_SEP_CLASS,
	S_HANGER,

	S_CHOOSE,
	S_SQRT,
	S_FRAC,
	S_MODULAR,

	S_ONE,
	S_ZERO,
	S_BIGNUM,
	S_SUM_CLASS,
	S_FUN_CLASS,
	S_EQ_CLASS,
	S_FLOAT,
	S_STAR,
	S_POS,
	S_NEG,
	S_VERT,
	S_FACT,
	S_PRIME,
	S_SUB_SCRIPT,
	S_SUP_SCRIPT,
	S_DOTS,
	S_PARTIAL,
	S_PI,
	S_INFTY, 
	S_EMPTY, 
	S_ANGLE, 
	S_PERP, 
	S_CIRC, 
	S_PERCENT, 
	S_VECT,
	S_CEIL,
	S_FLOOR,
	
	/* auto-generated symbol set */
	/* INSERT_HERE */
	S_sd_fun,

	/* number of non-number and non-alphabet symbols */
	S_N
};
