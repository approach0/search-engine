#include <string.h>
#include "tex-parser/tex-parser.h"
#include "math-expr-highlight.h"
#include "config.h"

#define MAX_COLOR_STRLEN 32

static char color_map[][MAX_COLOR_STRLEN] = {
	"Magenta",
	"Cyan",
	"Orange"
};

static int
gen_map(struct tex_parse_ret *ret, uint64_t *mask, int k,
        uint32_t *begin_end_map, uint32_t *begin_color_map)
{
	for (int t = 0; t < k; t++) {
		for (uint32_t i = 0; i < MAX_SUBPATH_ID; i++) {
			if ((mask[t] >> i) & 0x1) {
				uint32_t pathID = i + 1;
				uint32_t begin = ret->idposmap[pathID] >> 16;
				uint32_t end   = ret->idposmap[pathID] & 0xffff;
				if (end > MAX_TXT_SEG_BYTES)
					return 1;
				// printf("path#%u: %u, %u\n", i + 1, begin, end);
				begin_end_map[begin] = end;
				begin_color_map[begin] = t + 1;
			}
		}
	}

	return 0;
}

char *math_oprand_highlight(char* kw, uint64_t* mask, int k)
{
	static char output[MAX_TEX_STRLEN] = "";
	uint32_t begin_end_map[MAX_TEX_STRLEN] = {0};
	uint32_t begin_color_map[MAX_TEX_STRLEN];

	struct tex_parse_ret ret;
	ret = tex_parse(kw, strlen(kw), 0);

	if (gen_map(&ret, mask, k, begin_end_map, begin_color_map)) {
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
			size_t l = sizeof(color_map) / MAX_COLOR_STRLEN;
			char *color = color_map[begin_color_map[begin] % l];
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
	return output;
}
