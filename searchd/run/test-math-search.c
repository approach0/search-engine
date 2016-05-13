#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#include "list/list.h"
#include "keyval-db/keyval-db.h"
#include "postmerge.h"

#include "math-search.h"

/* for MAX_QUERY_BYTES */
#include "txt-seg/txt-seg.h"
#include "txt-seg/config.h"

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge_arg* pm_arg,
                      void* extra_args)
{
	uint32_t                    i, j, k;
	math_posting_t              posting;
	uint32_t                    pathinfo_pos;
	struct math_posting_item   *po_item;
	struct math_pathinfo_pack  *pathinfo_pack;
	struct math_pathinfo       *pathinfo;
	struct subpath_ele         *subpath_ele;
	mnc_score_t                 score;
	bool                        skipped = 0;

	/* get number of query leaf-root paths' pointer */
	P_CAST(qry_n_lr_paths, uint32_t, extra_args);
//	printf("query leaf-root paths: %u\n", *qry_n_lr_paths);

	/* reset mnc for scoring new document */
	uint32_t slot;
	struct mnc_ref mnc_ref;
	mnc_reset_docs();

	for (i = 0; i < pm_arg->n_postings; i++) {
		/* for each merged posting item from posting lists */
		posting = pm_arg->postings[i];
		po_item = pm_arg->cur_pos_item[i];
		subpath_ele = math_posting_get_ele(posting);
		assert(NULL != subpath_ele);

		/* get pathinfo position of corresponding merged item */
		pathinfo_pos = po_item->pathinfo_pos;

		/* use pathinfo position to get pathinfo packet */
		pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);
		assert(NULL != pathinfo_pack);

		if (*qry_n_lr_paths > pathinfo_pack->n_lr_paths) {
			/* impossible to match, skip this math expression */

			printf("query leaf-root paths (%u) is greater than "
			       "document leaf-root paths (%u), skip this expression."
			       "\n", *qry_n_lr_paths, pathinfo_pack->n_lr_paths);
			skipped = 1;
			break;
		}

		for (j = 0; j < pathinfo_pack->n_paths; j++) {
			/* for each pathinfo from this pathinfo packet */
			pathinfo = pathinfo_pack->pathinfo + j;

			/* preparing to score corresponding document subpaths */
			mnc_ref.sym = pathinfo->lf_symb;
			slot = mnc_map_slot(mnc_ref);

			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
				/* add this document subpath for scoring */
				mnc_doc_add_rele(slot, pathinfo->path_id,
				                 subpath_ele->dup[k]->path_id);
			}
		}
	}

	/* finally calculate expression similarity score */
	if (!skipped)
		score = mnc_score();
	else
		score = 0;

	printf("docID#%u expID#%u score: %u\n",
			po_item->doc_id, po_item->exp_id, score);
}

int main(int argc, char *argv[])
{
	int                     i, opt;
	math_index_t            mi = NULL;
	keyval_db_t             keyval_db = NULL;

	char       query[MAX_MERGE_POSTINGS][MAX_QUERY_BYTES];
	uint32_t   n_queries = 0;

	char      *index_path = NULL;
	const char kv_db_fname[] = "kvdb-offset.bin";

	while ((opt = getopt(argc, argv, "hp:t:o:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test for math search.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> | -t <TeX>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -t 'a+b'\n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			strcpy(query[n_queries ++], optarg);
			break;

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
	 * open math index.
	 */
	printf("opening math index ...\n");
	mi = math_index_open(index_path, MATH_INDEX_READ_ONLY);
	if (mi == NULL) {
		printf("cannot create/open math index.\n");
		goto exit;
	}

	/*
	 * open document offset key-value database.
	 */
	printf("opening document offset key-value DB...\n");
	keyval_db = keyval_db_open_under(kv_db_fname, index_path,
	                                 KEYVAL_DB_OPEN_RD);
	if (keyval_db == NULL) {
		printf("key-value DB open error.\n");
		goto exit;
	} else {
		printf("%lu records in key-value DB.\n",
		       keyval_db_records(keyval_db));
	}

	/* for now only single (query[0]) is searched */
	math_search_posting_merge(mi, query[0], DIR_MERGE_DEPTH_FIRST,
	                          &math_posting_on_merge, NULL);

exit:
	if (index_path)
		free(index_path);

	if (mi) {
		printf("closing math index...\n");
		math_index_close(mi);
	}

	if (keyval_db) {
		printf("closing key-value DB...\n");
		keyval_db_close(keyval_db);
	}

	return 0;
}
