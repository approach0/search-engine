#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "search.h"
#include "snippet.h"
#include "indexer/doc-tok-pos.h"

struct snippet_arg {
	keyval_db_t   kv_db;
	struct query  qry;
};

static char *get_doc_path(keyval_db_t kv_db, doc_id_t docID)
{
	char *doc_path;
	size_t val_sz;

	if(NULL == (doc_path = keyval_db_get(kv_db, &docID, sizeof(doc_id_t),
	                                     &val_sz))) {
		printf("get error: %s\n", keyval_db_last_err(kv_db));
		return NULL;
	}

	return doc_path;
}

struct calc_hl_position_arg {
	doc_id_t     docID;
	keyval_db_t  kv_db;
	list        *snippet_div_list;
};

static LIST_IT_CALLBK(calc_hl_position)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(chp_arg, struct calc_hl_position_arg, pa_extra);

	char          *kw_utf8 = wstr2mbstr(kw->wstr);
	docterm_t      docterm = {chp_arg->docID, ""};
	size_t         val_sz;
	doctok_pos_t  *termpos;

	/* get keyword position in document */
	strcpy(docterm.term, kw_utf8);
	termpos = keyval_db_get(chp_arg->kv_db, &docterm,
	                        sizeof(doc_id_t) + strlen(docterm.term), &val_sz);

	/* add this keyword to snippet */
	if(NULL != termpos) {
		snippet_add_pos(chp_arg->snippet_div_list, docterm.term,
				termpos->doc_pos, termpos->n_bytes);
		free(termpos);
	}

	LIST_GO_OVER;
}

static void
print_result_snippet(doc_id_t docID, struct snippet_arg *snp_arg)
{
	char *doc_path;
	FILE *doc_fh;
	list  snippet_div_list = LIST_NULL;
	struct calc_hl_position_arg chp_arg =
		{docID, snp_arg->kv_db, &snippet_div_list};

	/* get document file path */
	doc_path = get_doc_path(snp_arg->kv_db, docID);
	printf("@ %s\n", doc_path);

	/* calculate highlighted keyword positions in snippet */
	list_foreach(&snp_arg->qry.keywords, &calc_hl_position, &chp_arg);

	/* print highlighted snippet in terminal */
	doc_fh = fopen(doc_path, "r");
	if (doc_fh) {
		snippet_read_file(doc_fh, &snippet_div_list);
		fclose(doc_fh);

		snippet_hi_print(&snippet_div_list);
	}

	/* free related variables */
	snippet_free_div_list(&snippet_div_list);
	free(doc_path);
}

static void
print_result(struct rank_set *rs, struct rank_hit* hit,
               uint32_t cnt, void* arg)
{
	P_CAST(snp_arg, struct snippet_arg, arg);

	printf("result#%u: doc#%u score=%.3f\n", cnt, hit->docID, hit->score);
	print_result_snippet(hit->docID, snp_arg);
}

static uint32_t
print_results(struct rank_set *rs, struct snippet_arg *snp_arg)
{
	struct rank_wind win;
	uint32_t         total_pages, page = 0;
	const uint32_t   res_per_page = DEFAULT_RES_PER_PAGE;

	do {
		win = rank_window_calc(rs, page, res_per_page, &total_pages);
		if (win.to > 0) {
			printf("page#%u (from %u to %u):\n",
			       page + 1, win.from, win.to);
			rank_window_foreach(&win, &print_result, snp_arg);
			page ++;
		}
	} while (page < total_pages);

	return total_pages;
}

int main(int argc, char *argv[])
{
	struct query_keyword keyword;
	struct query         qry;
	struct indices       indices;
	int                  opt;
	char                *index_path = NULL;
	struct rank_set      rk_set;
	struct snippet_arg   snp_arg;

	/* initialize text segmentation module */
	text_segment_init("../jieba/fork/dict");

	/* a single new query */
	qry = query_new();

	while ((opt = getopt(argc, argv, "hp:t:m:x:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("testcase of search function.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -p <index path> |"
			       " -t <term> |"
			       " -m <tex> |"
			       " -x <text>"
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			keyword.type = QUERY_KEYWORD_TERM;
			wstr_copy(keyword.wstr, mbstr2wstr(optarg));
			query_push_keyword(&qry, &keyword);
			break;

		case 'm':
			keyword.type = QUERY_KEYWORD_TEX;
			keyword.pos = 0; /* do not care for now */
			wstr_copy(keyword.wstr, mbstr2wstr(optarg));
			query_push_keyword(&qry, &keyword);
			break;

		case 'x':
			query_digest_utf8txt(&qry, optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (index_path == NULL || qry.len == 0) {
		printf("not enough arguments.\n");
		goto exit;
	}

	/*
	 * print program arguments.
	 */
	printf("index path: %s\n", index_path);
	query_print_to(qry, stdout);

	/*
	 * open indices
	 */
	printf("opening index...\n");
	indices = indices_open(index_path);
	if (indices.open_err) {
		printf("index open failed.\n");
		goto close;
	}

	/* setup cache */
	indices_cache(&indices, 32 MB);

	/* search query */
	rk_set = indices_run_query(&indices, qry);

	/*
	 * print ranked search results in page(s).
	 */
	snp_arg.kv_db = indices.keyval_db;
	snp_arg.qry   = qry;
	printf("result(s): %u pages.\n",
	       print_results(&rk_set, &snp_arg));

	/* free ranking set (ranked hits / search results) */
	rank_set_free(&rk_set);

close:
	/*
	 * close indices
	 */
	printf("closing index...\n");
	indices_close(&indices);

exit:
	/*
	 * free program arguments
	 */
	if (index_path)
		free(index_path);

	/*
	 * free other program modules
	 */
	query_delete(qry);
	text_segment_free();
	return 0;
}
