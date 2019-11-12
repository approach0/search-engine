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
	mnc_score_qry_path_add(gmnc, _id); \
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
	 * Query:
	 * [1]: (a+b)/c + ca + cab
	 * [2]: a > c
	 *
	 * Document:
	 * [1]: (z+x+y)/c + cxy + czxy
	 * [2]: y > c
	 */

	/* make query [1] */
	QRY_PATH_ADD(lmnc1, id('a'));
	QRY_PATH_ADD(lmnc1, id('b'));
	QRY_PATH_ADD(lmnc1, id('c'));
	QRY_PATH_ADD(lmnc1, id('c'));
	QRY_PATH_ADD(lmnc1, id('a'));
	QRY_PATH_ADD(lmnc1, id('c'));
	QRY_PATH_ADD(lmnc1, id('a'));
	QRY_PATH_ADD(lmnc1, id('b'));

	/* make query [2] */
	QRY_PATH_ADD(lmnc2, id('a'));
	QRY_PATH_ADD(lmnc2, id('c'));

	/* only need to sort global mnc */
	mnc_score_qry_path_sort(gmnc);

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

	/* for query [2] */
	/*
	 * Query:
	 * [2]: a > c
	 *
	 * Document:
	 * [2]: y > c
	 */
	DOC_PATH_ADD(lmnc2, id('a'), id('y'), 0.9f);
	DOC_PATH_ADD(lmnc2, id('a'), id('c'), 0.9f);

	DOC_PATH_ADD(lmnc2, id('c'), id('y'), 0.9f);
	DOC_PATH_ADD(lmnc2, id('c'), id('c'), 1.0f);

	/* align global mnc */
	float s = mnc_score_align(gmnc);
	printf("global score: %f\n", s);

	/* calculate individual local mnc scores */
	s = mnc_score_calc(gmnc, lmnc1);
	printf("local[1] score: %f\n", s);

	s = mnc_score_calc(gmnc, lmnc2);
	printf("local[1] score: %f\n", s);
}

int main()
{
	struct mnc_scorer gmnc, lmnc1, lmnc2;

	/* initialization */
	mnc_score_init(&gmnc);
	mnc_score_init(&lmnc1);
	mnc_score_init(&lmnc2);

	/* test */
	test_case(&gmnc, &lmnc1, &lmnc2);

	/* reset */
	mnc_score_doc_reset(&gmnc);
	mnc_score_doc_reset(&lmnc1);
	mnc_score_doc_reset(&lmnc2);

	/* test again, see if reset works */
	test_case(&gmnc, &lmnc1, &lmnc2);

	/* free instances */
	mnc_score_free(&gmnc);
	mnc_score_free(&lmnc1);
	mnc_score_free(&lmnc2);
	mhook_print_unfree();
	return 0;
}
