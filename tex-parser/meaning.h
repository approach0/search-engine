static __inline__ int meaningful_gener(enum token_id id)
{
	/* degree tokens */
	if (id > T_NIL) {
		if (id < T_DEGREE_VALVE) {
			return 0;
		} else if (id == T_DEGREE_VALVE) {
			return 0;
		} else if (id <= T_MAX_RANK) {
			return 0;
		}
	}

	switch (id) {
	case T_TAB_ROW:
	case T_TAB_COL:
	case T_SEP:

	case T_BASE:
	case T_SUBSCRIPT:
	case T_SUPSCRIPT:
	case T_PRE_SUBSCRIPT:
	case T_PRE_SUPSCRIPT:

	case T_MATHML_ROOT:
	case T_MAX_RANK:
	case T_PARTIAL:
	case T_DEGREE_VALVE:
	case T_NIL:
		return 0;
	default:
		return 1;
	}
}
