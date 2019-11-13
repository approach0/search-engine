#include <stdio.h>
#include <stdlib.h>
#include "mhook/mhook.h"
#include "tex-parser/head.h"
#include "mnc-score.h"

static uint16_t id(char s)
{
	return s - 'a' + SYMBOL_ID_a2Z_BEGIN;
}

#define QRY_PATH_ADD(_lmnc, _id) \
	mnc_score_qry_path_add(&gmnc, _id); \
	mnc_score_qry_path_add(_lmnc, _id)

#define DOC_PATH_ADD(_lmnc, _qid, _did, _score) \
	mnc_score_doc_path_add(gmnc, _qid, _did, _score); \
	mnc_score_doc_path_add(_lmnc,_qid, _did, _score)

static void test_case(
	struct mnc_scorer *gmnc,
	struct mnc_scorer *lmnc1,
	struct mnc_scorer *lmnc2)
{
	/*
	 * match document symbols
	 */

	/* for query [1] */

	DOC_PATH_ADD(lmnc1, id('a'), id('z'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('a'), id('x'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('a'), id('y'), 0.9f);

	DOC_PATH_ADD(lmnc1, id('b'), id('z'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('b'), id('x'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('b'), id('y'), 0.9f);

	DOC_PATH_ADD(lmnc1, id('c'), id('c'), 1.f);

	/* two 'c' in top-level addition terms */
	DOC_PATH_ADD(lmnc1, id('c'), id('c'), 2.f);
	DOC_PATH_ADD(lmnc1, id('c'), id('x'), 1.8f);
	DOC_PATH_ADD(lmnc1, id('c'), id('y'), 1.8f);
	DOC_PATH_ADD(lmnc1, id('c'), id('z'), 0.9f);

	/* two 'a' in top-level addition terms */
	DOC_PATH_ADD(lmnc1, id('a'), id('c'), 1.8f);
	DOC_PATH_ADD(lmnc1, id('a'), id('x'), 1.8f);
	DOC_PATH_ADD(lmnc1, id('a'), id('y'), 1.8f);
	DOC_PATH_ADD(lmnc1, id('a'), id('z'), 0.9f);

	/* one 'b' in top-level addition terms */
	DOC_PATH_ADD(lmnc1, id('b'), id('c'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('b'), id('x'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('b'), id('y'), 0.9f);
	DOC_PATH_ADD(lmnc1, id('b'), id('z'), 0.9f);

	printf("lmnc1 after adding:\n");
	mnc_score_print(lmnc1, 0);

	/* for query [2] */

	DOC_PATH_ADD(lmnc2, id('a'), id('y'), 0.9f);
	DOC_PATH_ADD(lmnc2, id('a'), id('c'), 0.9f);

	DOC_PATH_ADD(lmnc2, id('c'), id('y'), 0.9f);
	DOC_PATH_ADD(lmnc2, id('c'), id('c'), 1.0f);

	printf("lmnc2 after adding:\n");
	mnc_score_print(lmnc2, 0);

	/* align global mnc */
	float s = mnc_score_align(gmnc);
	printf("global after adding and aligning:\n");
	mnc_score_print(gmnc, 1);
	printf("global score: %f\n", s);

	/* calculate individual local mnc scores */
	s = mnc_score_calc(gmnc, lmnc1);
	printf("local[1] score: %f / %u = %f\n", s, 11, s / 11);

	s = mnc_score_calc(gmnc, lmnc2);
	printf("local[2] score: %f / %u = %f\n", s, 2, s / 2);
}

int main()
{
	struct mnc_scorer gmnc, lmnc1, lmnc2;

	/* initialization */
	mnc_score_init(&gmnc);
	mnc_score_init(&lmnc1);
	mnc_score_init(&lmnc2);

	/*
	 * Query:
	 * [1]: (a+b)/c + ca + \sqrt{kkk} + cab
	 * [2]: a > c
	 *
	 * Document:
	 * [1]: (z+x+y)/c + cxy + czxy
	 * [2]: y > c
	 */

	/* make query [1] */
	QRY_PATH_ADD(&lmnc1, id('a'));
	QRY_PATH_ADD(&lmnc1, id('b'));
	QRY_PATH_ADD(&lmnc1, id('c'));
	QRY_PATH_ADD(&lmnc1, id('c'));
	QRY_PATH_ADD(&lmnc1, id('a'));
	QRY_PATH_ADD(&lmnc1, id('k'));
	QRY_PATH_ADD(&lmnc1, id('k'));
	QRY_PATH_ADD(&lmnc1, id('k'));
	QRY_PATH_ADD(&lmnc1, id('c'));
	QRY_PATH_ADD(&lmnc1, id('a'));
	QRY_PATH_ADD(&lmnc1, id('b'));
	printf("lmnc1: \n");
	mnc_score_print(&lmnc1, 0);

	/* make query [2] */
	QRY_PATH_ADD(&lmnc2, id('a'));
	QRY_PATH_ADD(&lmnc2, id('c'));
	printf("lmnc2: \n");
	mnc_score_print(&lmnc2, 0);

	/* only need to sort global mnc */
	mnc_score_qry_path_sort(&gmnc);
	printf("gmnc (sorted): \n");
	mnc_score_print(&gmnc, 0);

	/* test */
	test_case(&gmnc, &lmnc1, &lmnc2);
	printf("\n");

	/* reset */
	mnc_score_doc_reset(&gmnc);
	mnc_score_doc_reset(&lmnc1);
	mnc_score_doc_reset(&lmnc2);

	/* test again, see if reset works */
	test_case(&gmnc, &lmnc1, &lmnc2);
	printf("\n");

	/* free instances */
	mnc_score_free(&gmnc);
	mnc_score_free(&lmnc1);
	mnc_score_free(&lmnc2);
	mhook_print_unfree();
	return 0;
}
