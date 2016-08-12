#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "indexer/config.h" /* for MAX_CORPUS_FILE_SZ */
#include "indexer/index.h" /* for text_lexer and indices */
#include "mem-index/mem-posting.h"

#include "config.h"
#include "bm25-score.h"
#include "snippet.h"
#include "search-utils.h"

void
term_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	uint32_t i, j = 0;
	float tot_score, bm25_score = 0.f;

#ifdef ENABLE_PROXIMITY_SCORE
	float prox_score;
#endif

	doc_id_t docID = cur_min;

	P_CAST(pm_args, struct posting_merge_extra_args, extra_args);
	float doclen = (float)term_index_get_docLen(pm_args->indices->ti, docID);
	struct term_posting_item *pip /* posting item with positions */;

	for (i = 0; i < pm->n_postings; i++)
		if (pm->curIDs[i] == cur_min) {
			//printf("merge docID#%lu from posting[%d]\n", cur_min, i);
			pip = pm->cur_pos_item[i];

//			{ /* print position array */
//				int j;
//				position_t *pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);
//				for (j = 0; j < pip->tf; j++) {
//					printf("%u-", pos_arr[j]);
//				}
//				printf("\n");
//			}

			{
				/* set proximity input */
				position_t *pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);
				prox_set_input(pm_args->prox_in + j, pos_arr, pip->tf);
				j++;
			}

			bm25_score += BM25_term_i_score(pm_args->bm25args, i,
			                                pip->tf, doclen);
		}

#ifdef ENABLE_PROXIMITY_SCORE
	/* calculate overall score considering proximity. */
	prox_score = prox_calc_score(prox_min_dist(pm_args->prox_in, j));
	tot_score = bm25_score + prox_score;
#else
	tot_score = bm25_score;
#endif

//	printf("BM25 score = %f.\n", bm25_score);
//	printf("proximity score = %f.\n", prox_score);
//	printf("(total score: %f)\n", tot_score);

	consider_top_K(pm_args->rk_res, docID, tot_score,
	               pm_args->prox_in, j);
	return;
}

void print_res_item(struct rank_hit* hit, uint32_t cnt, void* arg)
{
	char  *str;
	size_t str_sz;
	list   highlight_list;
	P_CAST(indices, struct indices, arg);

	printf("result#%u: doc#%u score=%.3f\n", cnt, hit->docID, hit->score);

	/* get URL */
	str = get_blob_string(indices->url_bi, hit->docID, 0, &str_sz);
	printf("URL: %s" "\n\n", str);

	/* get document text */
	str = get_blob_string(indices->txt_bi, hit->docID, 1, &str_sz);

	/* prepare highlighter arguments */
	highlight_list = prepare_snippet(hit, str, str_sz, lex_eng_file);

	/* print snippet */
	snippet_hi_print(&highlight_list);
	printf("--------\n\n");

	/* free highlight list */
	snippet_free_highlight_list(&highlight_list);
}

uint32_t
print_all_rank_res(ranked_results_t *rk_res, struct indices *indices)
{
	struct rank_window win;
	uint32_t tot_pages, page = 0;

	do {
		win = rank_window_calc(rk_res, page, DEFAULT_RES_PER_PAGE, &tot_pages);
		if (win.to > 0) {
			printf("page#%u (from %u to %u):\n",
			       page + 1, win.from, win.to);
			rank_window_foreach(&win, &print_res_item, indices);
			page ++;
		}
	} while (page < tot_pages);

	return tot_pages;
}

static ranked_results_t
test_term_search(struct indices *indices, enum postmerge_op op,
                 char (*terms)[MAX_TERM_BYTES], uint32_t n_terms)
{
	uint32_t                        i;
	void                           *posting;
	term_id_t                       term_id;
	struct posting_merge_extra_args pm_args;
	ranked_results_t                rk_res;
	uint32_t                        docN, df;
	struct BM25_term_i_args         bm25args;
	struct postmerge                pm;
	struct postmerge_callbks       *calls = NULL;
	void                           *ti = indices->ti;

	/* testing in-memory posting */
	struct mem_posting *fork_posting;
	void *term_posting;

	const term_id_t forked_term_id = 1;
	printf("caching posting list[%u] of term `%s'...\n", forked_term_id,
	       term_lookup_r(ti, forked_term_id));

	term_posting = term_index_get_posting(ti, forked_term_id);
	fork_posting = postcache_fork_term_posting(term_posting);

	printf("forked posting list: \n");
	mem_posting_print_info(fork_posting);
	printf("\n");

	/* clear post merge structure */
	postmerge_posts_clear(&pm);

	/*
	 * for each term posting list, pre-calculate some scoring
	 * parameters and find associated posting list.
	 */
	docN = term_index_get_docN(ti);

	for (i = 0; i < n_terms; i++) {
		term_id = term_lookup(ti, terms[i]);
		if (term_id != 0) {

			df = term_index_get_df(ti, term_id);
			bm25args.idf[i] = BM25_idf((float)df, (float)docN);

			printf("term `%s' ID = %u, df[%d] = %u.\n",
			       terms[i], term_id, i, df);

			if (forked_term_id == term_id) {
				posting = fork_posting;
				calls = get_memory_postmerge_callbks();
				printf("using cached posting list.\n");
			} else {
				posting = term_index_get_posting(ti, term_id);
				calls = get_disk_postmerge_callbks();
				printf("using on-disk posting list.\n");
			}
		} else {
			posting = NULL;
			calls = NULL;

			df = 0;
			bm25args.idf[i] = BM25_idf((float)df, (float)docN);

			printf("term `%s' not found, df[%d] = %u.\n",
			       terms[i], i, df);
		}

		postmerge_posts_add(&pm, posting, calls, NULL);
	}

	/*
	 * prepare some scoring parameters.
	 */
	bm25args.n_postings = i;
	bm25args.avgDocLen = (float)term_index_get_avgDocLen(ti);
	bm25args.b  = BM25_DEFAULT_B;
	bm25args.k1 = BM25_DEFAULT_K1;
	bm25args.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args.avgDocLen;

	printf("BM25 arguments:\n");
	BM25_term_i_args_print(&bm25args);

	/*
	 * initialize ranking queue
	 */
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	/*
	 * merge extra arguments.
	 */
	pm_args.indices = indices;
	pm_args.bm25args = &bm25args;
	pm_args.rk_res = &rk_res;
	pm_args.prox_in = malloc(sizeof(prox_input_t) * pm.n_postings);

	/*
	 * merge and score.
	 */
	printf("start merging...\n");

	/*
	 * pause and continue on key press to have an idea
	 * of how long the actual search process takes.
	 */
	printf("Press Enter to Continue");
	while(getchar() != '\n');

	/* posting merge */
	if (!posting_merge(&pm, op, &term_posting_on_merge, &pm_args))
		fprintf(stderr, "posting merge operation undefined.\n");

	/* free proximity pointer array */
	free(pm_args.prox_in);

	/* free test in-memory posting */
	mem_posting_free(fork_posting);

	/*
	 * rank top K hits.
	 */
	priority_Q_sort(&rk_res);

	return rk_res;
}

int main(int argc, char *argv[])
{
	struct indices          indices;
	int                     opt;
	enum postmerge_op       op = POSTMERGE_OP_AND;

	static char query[MAX_MERGE_POSTINGS][MAX_TERM_BYTES];
	uint32_t   i, n_queries = 0;

	char      *index_path = NULL;

	uint32_t           res_pages;
	ranked_results_t   results;

	/* open text segmentation dictionary */
	printf("opening dictionary...\n");
	text_segment_init("");

	while ((opt = getopt(argc, argv, "hp:t:o:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test for merge postings.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> | -t <term> | -o <op>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -t 'nick ' -t 'wilde' -o OR\n", argv[0]);
			printf("%s -p ./tmp -t 'give ' -t 'up' -t 'dream' -o AND\n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			strcpy(query[n_queries ++], optarg);
			break;

		case 'o':
		{
			if (strcmp(optarg, "AND") == 0)
				op = POSTMERGE_OP_AND;
			else if (strcmp(optarg, "OR") == 0)
				op = POSTMERGE_OP_OR;
			else
				op = POSTMERGE_OP_UNDEF;

			break;
		}

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (index_path == NULL || n_queries == 0) {
		printf("not enough arguments.\n");
		goto exit;
	}

	/*
	 * print program arguments.
	 */
	printf("index path: %s\n", index_path);
	printf("query: ");
	for (i = 0; i < n_queries; i++) {
		printf("`%s'", query[i]);
		if (i + 1 != n_queries)
			printf(", ");
		else
			printf(".");
	}
	printf("\n");

	/*
	 * open indices.
	 */
	printf("opening index...\n");
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("indices open failed.\n");
		goto exit;
	}

	/*
	 * perform search.
	 */
	printf("searching terms...\n");
	results = test_term_search(&indices, op, query, n_queries);

	/*
	 * print ranked search results by page number.
	 */
	res_pages = print_all_rank_res(&results, &indices);
	printf("result(s): %u pages.\n", res_pages);

	/*
	 * free ranked results
	 */
	free_ranked_results(&results);

	/*
	 * close indices.
	 */
	printf("closing index...\n");
	indices_close(&indices);

exit:
	text_segment_free();

	if (index_path)
		free(index_path);

	return 0;
}
