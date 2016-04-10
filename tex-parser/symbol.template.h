enum symbol_id {
	/* nil symbol */
	S_NIL,

	/* special symbols */
	S_bignum,
	S_zero,
	S_one,
	S_float,
	S_hanger,
	S_base,

	/* pair symbols */
	S_bracket,
	S_array,
	S_angle,
	S_slash,
	S_hair,
	S_arrow,
	S_ceil,
	S_floor,

	/* auto-generated symbol set */
	/* INSERT_HERE */

	/* number of non-number and non-alphabet symbols */
	S_N
};
