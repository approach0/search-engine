#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"

/*
 * query related functions.
 */

LIST_DEF_FREE_FUN(query_list_free, struct query_keyword, ln, free(p));

struct query query_new()
{
	struct query qry;
	LIST_CONS(qry.keywords);
	qry.len = 0;
	return qry;
}

void query_delete(struct query qry)
{
	query_list_free(&qry.keywords);
	qry.len = 0;
}

static LIST_IT_CALLBK(qry_keyword_print)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(fh, FILE, pa_extra);

	fprintf(fh, "[%u]: `%S'", kw->pos, kw->wstr);

	if (kw->type == QUERY_KEYWORD_TEX)
		fprintf(fh, " (tex)");

	if (pa_now->now == pa_head->last)
		fprintf(fh, ".");
	else
		fprintf(fh, ", ");

	LIST_GO_OVER;
}

void query_print_to(struct query qry, FILE* fh)
{
	fprintf(fh, "query: ");
	list_foreach(&qry.keywords, &qry_keyword_print, fh);
	fprintf(fh, "\n");
}

void query_push_keyword(struct query *qry, const struct query_keyword* kw)
{
	struct query_keyword *copy = malloc(sizeof(struct query_keyword));

	memcpy(copy, kw, sizeof(struct query_keyword));
	LIST_NODE_CONS(copy->ln);

	list_insert_one_at_tail(&copy->ln, &qry->keywords, NULL, NULL);
	copy->pos = (qry->len ++);
}

static LIST_IT_CALLBK(add_into_qry)
{
	LIST_OBJ(struct term_list_node, nd, ln);
	P_CAST(qry, struct query, pa_extra);
	struct query_keyword kw;

	kw.type = QUERY_KEYWORD_TERM;
	wstr_copy(kw.wstr, nd->term);

	query_push_keyword(qry, &kw);
	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(txt_seg_list_free, struct term_list_node,
                  ln, free(p));

void query_digest_utf8txt(struct query *qry, const char* txt)
{
	list li = text_segment(txt);
	list_foreach(&li, &add_into_qry, qry);
	txt_seg_list_free(&li);
}

/*
 * indices related functions.
 */

struct indices indices_open(const char*index_path)
{
	struct indices indices;
	bool           open_err = 0;
	const char     kv_db_fname[] = "kvdb-offset.bin";
	char           term_index_path[MAX_DIR_PATH_NAME_LEN];

	void         *ti = NULL;
	math_index_t  mi = NULL;
	keyval_db_t   keyval_db = NULL;
	
	/*
	 * open term index.
	 */
	sprintf(term_index_path, "%s/term", index_path);
	mkdir_p(term_index_path);
	ti = term_index_open(term_index_path, TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
//		printf("cannot create/open term index.\n");

		open_err = 1;
		goto skip;
	}

	/*
	 * open math index.
	 */
	mi = math_index_open(index_path, MATH_INDEX_READ_ONLY);
	if (mi == NULL) {
//		printf("cannot create/open math index.\n");

		open_err = 1;
		goto skip;
	}

	/*
	 * open document offset key-value database.
	 */
	keyval_db = keyval_db_open_under(kv_db_fname, index_path,
	                                 KEYVAL_DB_OPEN_RD);
	if (keyval_db == NULL) {
//		printf("key-value DB open error.\n");

		open_err = 1;
		goto skip;
	} else {
		printf("%lu records in key-value DB.\n",
		       keyval_db_records(keyval_db));
	}

skip:
	indices.ti = ti;
	indices.mi = mi;
	indices.keyval_db = keyval_db;
	indices.open_err = open_err;
	return indices;
}

void indices_close(struct indices* indices)
{
	if (indices->ti) {
		term_index_close(indices->ti);
		indices->ti = NULL;
	}

	if (indices->mi) {
		math_index_close(indices->mi);
		indices->mi = NULL;
	}

	if (indices->keyval_db) {
		keyval_db_close(indices->keyval_db);
		indices->keyval_db = NULL;
	}
}

void indices_run_query(struct indices *indices, const struct query qry)
{
	return;
}
