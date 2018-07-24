#include <string.h>
#include "tex-parser/tex-parser.h"
#include "txt-seg/config.h"
#include "math-expr-highlight.h"

#define MAX_COLOR_STRLEN 32

static char color_map[][MAX_COLOR_STRLEN] = {
	"Magenta",
	"Cyan",
	"Orange"
};

static void
gen_map(struct tex_parse_ret *ret, uint64_t *mask, int k,
        uint32_t *begin_end_map, uint32_t *begin_color_map)
{
	for (int t = 0; t < k; t++) {
		for (uint32_t i = 0; i < MAX_SUBPATH_ID; i++) {
			if ((mask[t] >> i) & 0x1) {
				uint32_t pathID = i + 1;
				uint32_t begin = ret->idposmap[pathID] >> 16;
				uint32_t end   = ret->idposmap[pathID] & 0xffff;
				begin_end_map[begin] = end;
				begin_color_map[begin] = t + 1;
				// printf("path#%u: %u, %u\n", i + 1, begin, end);
			}
		}
	}

}

char *math_oprand_highlight(char* kw, uint64_t* mask, int k)
{
	uint32_t begin_end_map[MAX_TXT_SEG_BYTES] = {0};
	uint32_t begin_color_map[MAX_TXT_SEG_BYTES];

	struct tex_parse_ret ret;
	ret = tex_parse(kw, strlen(kw), 0);

	gen_map(&ret, mask, k, begin_end_map, begin_color_map);

	/* generate highlighted math TeX string */
	static char output[MAX_TXT_SEG_BYTES];
	int cur = 0;
	output[0] = '\0';
	char operand[MAX_TXT_SEG_BYTES];
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
