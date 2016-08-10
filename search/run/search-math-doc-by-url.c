#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "tex-parser/head.h"
#include "indexer/config.h"
#include "indexer/index.h"

#include "search-utils.h"
#include "math-expr-search.h"

static struct indices indices;

static void
print_pathinfo(math_posting_t *po, uint32_t pathinfo_pos)
{
	uint32_t i;
	struct math_pathinfo_pack *pathinfo_pack;
	struct math_pathinfo      *pathinfo;

	pathinfo_pack = math_posting_pathinfo(po, pathinfo_pos);
	if (NULL == pathinfo_pack) {
		printf("fails to read math posting pathinfo.\n");
		return;
	}

	/* upon success, print path info items */
	for (i = 0; i < pathinfo_pack->n_paths; i++) {
		pathinfo = pathinfo_pack->pathinfo + i;
		printf("[%u %s %x]",
		        pathinfo->path_id,
		        trans_symbol(pathinfo->lf_symb),
		        pathinfo->fr_hash);
	}
}

static void math_posting_find_expr(char* path, char *url)
{
	math_posting_t            *po;
	struct math_posting_item  *po_item;
	uint32_t                   pathinfo_pos;
	char                      *doc_url;
	size_t                     doc_url_sz;

	/* allocate memory for posting reader */
	po = math_posting_new_reader(NULL, path);

	/* start reading posting list (try to open file) */
	if (!math_posting_start(po)) {
		printf("cannot start reading posting list.\n");
		goto free;
	}

	do {
		/* read posting list item */
		po_item = math_posting_current(po);
		pathinfo_pos = po_item->pathinfo_pos;
		doc_url = get_blob_string(indices.url_bi, po_item->doc_id,
		                          false, &doc_url_sz);

		if (0 == strcmp(doc_url, url)) {
			printf("doc#%u, exp#%u;",
					po_item->doc_id, po_item->exp_id);
			print_pathinfo(po, pathinfo_pos);
			printf("\n");
		}

	} while (math_posting_next(po));

free:
	math_posting_finish(po);
	math_posting_free_reader(po);
}

static LIST_IT_CALLBK(find_url)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(url, char, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN];

	math_index_mk_path_str(indices.mi, ele->dup[0], path);
	printf("searching path: %s\n", path);
	math_posting_find_expr(path, url);

	LIST_GO_OVER;
}

static void
where_is_expr(char *tex, char *url)
{
	struct tex_parse_ret parse_ret;
	list subpath_set = LIST_NULL;

	printf("finding doc of URL: %s\n", url);
	printf("containing tex: %s\n", tex);

	/* parse TeX */
	parse_ret = tex_parse(tex, 0, true);

	if (parse_ret.code != PARSER_RETCODE_ERR) {
		optr_print(parse_ret.operator_tree, stdout);

		printf("subpaths:\n");
		subpaths_print(&parse_ret.subpaths, stdout);

		subpath_set_from_subpaths(&parse_ret.subpaths,
		                          &subpath_set);

		printf("subpath set:\n");
		subpath_set_print(&subpath_set, stdout);
		printf("\n");

		list_foreach(&subpath_set, find_url, url);
		printf("\n");

		subpaths_release(&parse_ret.subpaths);
	} else {
		printf("parser error: %s\n", parse_ret.msg);
	}
}

int main(int argc, char *argv[])
{
	int           opt;
	char         *index_path = NULL;
	char          tex[MAX_QUERY_BYTES] = {0};
	static char   url[MAX_CORPUS_FILE_SZ] = {0};

	while ((opt = getopt(argc, argv, "hi:t:u:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("Debug tool to find out why a math "
			       "expression is not in search results.");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -i <index path> |"
			       " -t <TeX> |"
			       " -u <URL> |",
			       argv[0]);
			printf("\n");
			goto exit;

		case 'i':
			index_path = strdup(optarg);
			break;

		case 't':
			strcpy(tex, optarg);
			break;

		case 'u':
			strcpy(url, optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (index_path == NULL || tex[0] == 0 || url[0] == 0) {
		printf("not enough arguments.\n");
		goto exit;
	}

	/*
	 * open math index.
	 */
	printf("opening indices ...\n");
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("cannot create/open index.\n");
		goto exit;
	}

	/* search */
	where_is_expr(tex, url);

exit:
	if (index_path)
		free(index_path);

	printf("closing indices...\n");
	indices_close(&indices);

	return 0;
}
