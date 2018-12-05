enum math_font_t {
	MFONT_NORMAL,
	MFONT_CALLI, /* i.e. calligraphy */
	MFONT_BLACKBOARD, /* e.g. bbold */
	MFONT_FRAK, /* Fraktur (aka Gothic) */
	MFONT_N /* number of fonts */
};

static __inline__ const char *math_font_name(enum math_font_t font)
{
	switch (font) {
	case MFONT_NORMAL:
		return "normal";
	case MFONT_FRAK:
		return "fraktur";
	case MFONT_CALLI:
		return "calligraphy";
	case MFONT_BLACKBOARD:
		return "blackboard-bold";
	default:
		return "undefined";
	}
}
