#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "parson/parson.h"
#include "sds/sds.h"
#include "postlist/math-postlist.h"

#include "indexer/config.h"
#include "indexer/index.h"

#include "search/config.h"
#include "search/search-utils.h"
#include "search/math-expr-search.h"
#include "search/math-prefix-qry.h"
#include "search/math-expr-sim.h"

#include "highlight/math-expr-highlight.h"

#include "config.h"

char *get_expr_by_pos(char*, size_t, uint32_t);

static struct math_postlist_item*
math_expr_prefix_alignmap(
	uint64_t cur_min, struct postmerge *pm,
	struct math_extra_score_arg *mesa,
	uint32_t *qmap, uint32_t *dmap
)
{
	struct math_postlist_item *item = NULL;
	struct math_prefix_qry    *pq = &mesa->pq;
	for (uint32_t i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			PTR_CAST(mepa, struct math_extra_posting_arg, pm->posting_args[i]);
			item = (struct math_postlist_item*)POSTMERGE_CUR(pm, i);

			for (uint32_t j = 0; j <= mepa->ele->dup_cnt; j++) {
				uint32_t qr, ql;
				qr = mepa->ele->rid[j];
				ql = mepa->ele->dup[j]->leaf_id;

				for (uint32_t k = 0; k < item->n_paths; k++) {
					uint32_t dr, dl;
					dr = item->subr_id[k];
					dl = item->leaf_id[k];

					uint64_t res = pq_hit(pq, qr, ql, dr, dl);
					(void)res;
//					if (0 == res && item->doc_id == 219890) {
//						printf("r%u~l%u hit r%u~l%u\n", qr, ql, dr, dl);
//					}
				}
			}
		}
	}

	pq_align_map(pq, qmap, dmap);
	pq_reset(pq);

	return item;
}

struct highlight_args {
	struct indices *indices;
	uint32_t doc_id, exp_id;
	const char *query_tex;
	sds *qout, *dout;
};

static void _debug_print_map(uint32_t *map, int n, int color)
{
	for (int i = 0; i < n; i++)
		if (color == map[i])
			printf("node#%d --> color%u\n", i, color);
}

static int
math_highlight_on_merge(uint64_t cur_min, struct postmerge* pm, void* args)
{
	PTR_CAST(mesa, struct math_extra_score_arg, args);
	PTR_CAST(hila, struct highlight_args, mesa->expr_srch_arg);

	struct math_postlist_item *item = NULL;
	uint32_t qmap[MAX_NODE_IDS] = {0};
	uint32_t dmap[MAX_NODE_IDS] = {0};

	item = math_expr_prefix_alignmap(cur_min, pm, mesa, qmap, dmap);

//	if (item && item->doc_id == 219890) {
//		_debug_print_map(qmap, MAX_NODE_IDS, 2);
//		_debug_print_map(dmap, MAX_NODE_IDS, 2);
//	}

	if (item && item->doc_id == hila->doc_id && item->exp_id == hila->exp_id) {
		size_t txt_sz;
		char *txt = get_blob_string(hila->indices->txt_bi, item->doc_id, 1, &txt_sz);
		char *doc_tex = get_expr_by_pos(txt, txt_sz, item->exp_id);
		free(txt);

		math_tree_highlight((char*)hila->query_tex, qmap, MAX_MTREE, hila->qout);
		math_tree_highlight(doc_tex, dmap, MAX_MTREE, hila->dout);

		return 1;
	}

	return 0;
}

static const char*
graph_query_response(struct indices *indices,
                     const char *q, uint32_t doc_id, uint32_t exp_id)
{
	static char ret[MAX_SNIPPET_SZ];
	sds q_out = sdsempty();
	sds d_out = sdsempty();
	struct highlight_args args = {
		indices, doc_id, exp_id, q, &q_out, &d_out
	};

	math_postmerge(indices, (char*)q, MATH_SRCH_FUZZY_STRUCT,
	               &math_highlight_on_merge, &args);
	char *esc_q_out = json_encode_string(q_out);
	char *esc_d_out = json_encode_string(d_out);

	sprintf(ret, "{\"qry\": %s, \"doc\": %s}", esc_q_out, esc_d_out);

	free(esc_q_out);
	free(esc_d_out);
	sdsfree(q_out);
	sdsfree(d_out);
	return (const char*)ret;
}

static const char*
operands_query_response(const char *tex, uint64_t *masks, int k)
{
	static char ret[MAX_SNIPPET_SZ];
	const char *hi_tex = math_oprand_highlight((char*)tex, masks, k);

	char *esc_hi_tex = json_encode_string(hi_tex);
	sprintf(ret, "{\"hi_tex\": %s}", esc_hi_tex);
	free(esc_hi_tex);

	return (const char*)ret;
}

static const char *
httpd_on_recv(const char* req, void* arg_)
{
	P_CAST(indices, struct indices, arg_);
	const char *ret = NULL;
	JSON_Object *parson_obj;

	JSON_Value *parson_val = json_parse_string(req);
	if (parson_val == NULL) {
		fprintf(stderr, "Parson fails to parse JSON query.\n");
		return ret;
	}

	/* get query page */
	parson_obj = json_value_get_object(parson_val);

	if (!json_object_has_value_of_type(parson_obj, "query_type",
	                                   JSONString)) {
		fprintf(stderr, "JSON query has no type.\n");
		goto retjson;
	}

	const char *query_type = json_object_get_string(parson_obj, "query_type");

	if (0 == strcmp(query_type, "graph")) {
		const char* q = json_object_get_string(parson_obj, "query");
		uint32_t doc_id = json_object_get_number(parson_obj, "doc_id");
		uint32_t exp_id = json_object_get_number(parson_obj, "exp_id");

		printf("request: tree highlight for doc#%u, exp#%u\n", doc_id, exp_id);
		ret = graph_query_response(indices, q, doc_id, exp_id);

	} else if (0 == strcmp(query_type, "operands")) {
		const char* tex = json_object_get_string(parson_obj, "tex");
		uint64_t masks[MAX_MTREE];
#define json_getstr(_prop) json_object_get_string(parson_obj, _prop)
		sscanf(json_getstr("mask0"), "%lx", masks + 0);
		sscanf(json_getstr("mask1"), "%lx", masks + 1);
		sscanf(json_getstr("mask2"), "%lx", masks + 2);

		printf("request: operands highlight for '%s' (mask: %lx,%lx,%lx)\n",
		       tex, masks[0], masks[1], masks[2]);
		ret = operands_query_response(tex, masks, MAX_MTREE);
	}

retjson:
	json_value_free(parson_val);
	return ret;
}

int main(int argc, char *argv[])
{
	int opt;
	char *index_path = NULL;
	struct indices indices;
	const int port = HIGHLIGHTD_DEFAULT_PORT;

	/* parse program arguments */
	while ((opt = getopt(argc, argv, "hi:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("search daemon.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -i <index path> "
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'i':
			index_path = strdup(optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/* check program arguments */
	if (index_path == NULL) {
		fprintf(stderr, "indices path not specified.\n");
		goto exit;
	}

	/* open indices */
	printf("opening index at: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	/* run httpd */
	printf("listen on port %hu\n", port);

	struct uri_handler uri_handlers[] = {{HIGHLIGHTD_DEFAULT_URI , &httpd_on_recv}};
	httpd_run(port, uri_handlers, 1, &indices);

close:
	/* close indices */
	printf("closing index...\n");
	indices_close(&indices);

exit:
	/*
	 * free program arguments
	 */
	free(index_path);

	mhook_print_unfree();
	return 0;
}
