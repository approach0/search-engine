#define SYMBOL_ID_VOLUME UINT16_MAX /* two-byte ID space */

/*
 * Below are the symbol ID space.
 */

/* first is enumerated symbols (extracted from lexer) */
#define SYMBOL_ID_ENUM_BEGIN      S_NIL
#define SYMBOL_ID_ENUM_END        (S_N - 1)
/* second is letter a-Z in different fonts */
#define SYMBOL_ID_a2Z_BEGIN       (SYMBOL_ID_ENUM_END + 1)
#define SYMBOL_ID_a2Z_END         (SYMBOL_ID_a2Z_BEGIN + MFONT_N * (26 * 2) - 1)
/* third is small numbers: [0, N)*/
#define SYMBOL_ID_NUM_BEGIN       (SYMBOL_ID_a2Z_END + 1)
#define SYMBOL_ID_NUM_END         (SYMBOL_ID_VOLUME - 1)
/* third is large numbers: [N, infty) */
#define SYMBOL_ID_LARGE_NUM_BEGIN SYMBOL_ID_VOLUME
#define SYMBOL_ID_LARGE_NUM_FIRST (SYMBOL_ID_NUM_END - SYMBOL_ID_NUM_BEGIN + 1) /* N */

#include <stdio.h>
static __inline__ void math_symb_space_print()
{
	printf("enumerate symbols space: [%u, %u]\n", SYMBOL_ID_ENUM_BEGIN, SYMBOL_ID_ENUM_END);
	printf("a-Z symbols space: [%u, %u]\n", SYMBOL_ID_a2Z_BEGIN, SYMBOL_ID_a2Z_END);
	printf("small number space: [%u, %u]\n", SYMBOL_ID_NUM_BEGIN, SYMBOL_ID_NUM_END);
	printf("large number space: [%u, infty), actually S_bignum\n", SYMBOL_ID_LARGE_NUM_BEGIN);
	printf("the first large number: %u\n", SYMBOL_ID_LARGE_NUM_FIRST);
}
