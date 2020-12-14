#include <string.h>
#include "tex-parser/head.h"
#include "math-expr-highlight.h"
#include "config.h"

static char *colors[] = {"Maroon", "Green", "Orange"};

static int
gen_map(uint32_t *idposmap, uint64_t *mask, int k,
        uint32_t *begin_end_map, uint32_t *begin_color_map)
{
	for (int t = 0; t < k; t++) {
		for (uint32_t i = 0; i < MAX_SUBPATH_ID; i++) {
			if ((mask[t] >> i) & 0x1L) {
				uint32_t begin = idposmap[i] >> 16;
				uint32_t end   = idposmap[i] & 0xffff;
				if (end > MAX_TXT_SEG_BYTES)
					return 1;
				begin_end_map[begin] = end;
				begin_color_map[begin] = t;
			}
		}
	}

	return 0;
}

void rm_limits(char *str)
{
	char *p;
	while ((p = strstr(str, "\\limits"))) {
		memset(p, ' ', strlen("\\limits"));
	}
}

char *math_oprand_highlight(char* kw, uint64_t* mask, int k)
{
	static char output[MAX_TEX_STRLEN] = "";
	uint32_t begin_end_map[MAX_TEX_STRLEN] = {0};
	uint32_t begin_color_map[MAX_TEX_STRLEN];

	struct tex_parse_ret ret;
	uint32_t idposmap[MAX_SUBPATH_ID];
	ret = tex_parse(kw);

	if (ret.code == PARSER_RETCODE_ERR ||
	    ret.operator_tree == NULL) {
		fprintf(stderr, "parse error in math_oprand_highlight()\n");
		return kw;
	}

	optr_gen_idpos_map(idposmap, ret.operator_tree);
	optr_release((struct optr_node*)ret.operator_tree);

	/* no need to keep paths here */
	subpaths_release(&ret.lrpaths);

	if (gen_map(idposmap, mask, k, begin_end_map, begin_color_map)) {
		fprintf(stderr, "Error in gen_map() \n");
		return kw;
	}

	/* generate highlighted math TeX string */
	int cur = 0; /* current position in output buffer */
	char operand[MAX_TXT_SEG_BYTES]; /* operand TeX buffer */
	for (size_t begin = 0; begin < strlen(kw); begin++) {
		uint32_t end = begin_end_map[begin];
		if (end) {
			/* an operand that needs to be highlighted */
			size_t l = sizeof(colors) / sizeof(char*);
			char *color = colors[begin_color_map[begin] % l];
			snprintf(operand, end - begin + 1, "%s", kw + begin);
			cur += sprintf(output + cur, "{\\color{%s}%s}", color, operand);
			begin = end - 1;
		} else {
			/* other characters */
			output[cur] = kw[begin];
			cur ++;
		}
	}
	output[cur] = '\0';

	/*
	 * If lim gets highlighted, then the following `limits' will complain
	 * about "limits is allowed only on operators" in MathJaX, because the
	 * lim surrounded by \color{} is not considered operator anymore.
	 */
	rm_limits(output);
	return output;
}

int math_tree_highlight(char *tex, uint32_t *map, int k, sds *o)
{
	struct tex_parse_ret ret;
	ret = tex_parse(tex);

	if (ret.code != PARSER_RETCODE_ERR && ret.operator_tree) {
		optr_graph_print(ret.operator_tree, colors, map, k, o);

		optr_release(ret.operator_tree);
		subpaths_release(&ret.lrpaths);
		return 0;
	}

	return 1;
}
