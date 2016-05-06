#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "indexer.h"

static void *term_index = NULL;
static math_index_t math_index = NULL;
static keyval_db_t keyval_db = NULL;

static doc_id_t new_docID = 0;
static doc_id_t new_expID = 0;
static list splited_terms = LIST_NULL;

static LIST_IT_CALLBK(index_term)
{
	LIST_OBJ(struct splited_term, st, ln);

//	printf("term `%s': ", st->term);
//	printf("%u[%u]\n", st->doc_pos, st->n_bytes);

	/* add term for inverted-index */
	term_index_doc_add(term_index, st->term);
	LIST_GO_OVER;
}

static LIST_IT_CALLBK(index_term_pos)
{
	LIST_OBJ(struct splited_term, st, ln);
	P_CAST(nterms, size_t, pa_extra);
	docterm_pos_t termpos = {st->doc_pos, st->n_bytes};
	docterm_t docterm;

	(*nterms) ++;

	/* save term position */
	docterm.docID = new_docID;
	strcpy(docterm.term, st->term);

	if(keyval_db_put(keyval_db,
	                 &docterm,sizeof(doc_id_t) + st->n_bytes,
	                 &termpos, sizeof(docterm_pos_t))) {
		printf("put error: %s\n", keyval_db_last_err(keyval_db));
		return LIST_RET_BREAK;
	}

	LIST_GO_OVER;
}

extern void handle_math(struct lex_slice *slice)
{
//	printf("math: %s (len=%u)\n", slice->mb_str, slice->offset);
	struct tex_parse_ret parse_ret;
	parse_ret = tex_parse(slice->mb_str, 0, false);

	if (parse_ret.code == PARSER_RETCODE_SUCC) {
		math_index_add_tex(math_index, new_docID, new_expID,
		                   parse_ret.subpaths);
		subpaths_release(&parse_ret.subpaths);
		new_expID ++;
	} else {
		printf("math parser error: %s\n", parse_ret.msg);
	}
}

extern void handle_text(struct lex_slice *slice)
{
//	printf("slice<%u,%u>: %s\n",
//	       slice->begin, slice->offset, slice->mb_str);

	/* convert english words to lower case */
	eng_to_lower_case(slice);

	/* split slice into terms */
	split_into_terms(slice, &splited_terms);
}

static void index_txt_document(const char *fullpath)
{
	size_t val_sz = strlen(fullpath) + 1;
	LIST_CONS(splited_terms);
	size_t nterms = 0;

	term_index_doc_begin(term_index);
	lex_txt_file(fullpath);
	list_foreach(&splited_terms, &index_term, NULL);
	new_docID = term_index_doc_end(term_index);

	new_expID = 0; /* clear expression ID */

	list_foreach(&splited_terms, &index_term_pos, &nterms);
	//printf("{%lu}", nterms);
	release_splited_terms(&splited_terms);

	/* save document path */
	if(keyval_db_put(keyval_db, &new_docID, sizeof(doc_id_t),
	                 (void *)fullpath, val_sz)) {
		printf("put error: %s\n", keyval_db_last_err(keyval_db));
		return;
	}
}

static int foreach_file_callbk(const char *filename, void *arg)
{
	char *path = (char*) arg;
	//char *ext = filename_ext(filename);
	char fullpath[MAX_FILE_NAME_LEN];

	if (1 /*ext && strcmp(ext, ".txt") == 0 */) {
		sprintf(fullpath, "%s/%s", path, filename);

		//printf("[file] %s\n", fullpath);
		index_txt_document(fullpath);
		if (term_index_maintain(term_index)) {
			printf("\r[term index merging...]");
			keyval_db_flush(keyval_db);
		}
	}

	return 0;
}

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	printf("[directory] %s\n", path);
	foreach_files_in(path, &foreach_file_callbk, (void*)path);
	return DS_RET_CONTINUE;
}

int main(int argc, char* argv[])
{
	int opt;
	char *path = NULL /* corpus path */;
	const char index_path[] = "./tmp";
	const char kv_db_fname[] = "kvdb-offset.bin";
	char term_index_path[MAX_FILE_NAME_LEN];

	while ((opt = getopt(argc, argv, "hp:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index txt document from a specified path. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <corpus path>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./some/where/file.txt\n", argv[0]);
			printf("%s -p ./some/where\n", argv[0]);
			goto exit;

		case 'p':
			path = strdup(optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (path) {
		printf("corpus path %s\n", path);
	} else {
		printf("no corpus path specified.\n");
		goto exit;
	}

	printf("opening dict...\n");
	text_segment_init("../jieba/fork/dict");
	text_segment_insert_usrterm("当且仅当");
	printf("dict opened.\n");

	printf("opening term index...\n");
	sprintf(term_index_path, "%s/term", index_path);

	mkdir_p(term_index_path);

	/* open term index */
	term_index = term_index_open(term_index_path, TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		printf("cannot create/open term index.\n");
		goto exit;
	}

	/* open math index */
	math_index = math_index_open(index_path, MATH_INDEX_WRITE);
	if (NULL == math_index) {
		printf("cannot create/open math index.\n");
		goto exit;
	}

	/* open document offset key-value database */
	keyval_db = keyval_db_open_under(kv_db_fname, index_path,
	                                 KEYVAL_DB_OPEN_WR);
	if (keyval_db == NULL) {
		printf("cannot create/open key-value DB.\n");
		goto exit;
	}

	/* start indexing */
	if (file_exists(path)) {
		printf("[single file] %s\n", path);
		index_txt_document(path);
	} else if (dir_exists(path)) {
		dir_search_podfs(path, &dir_search_callbk, NULL);
	} else {
		printf("not file/directory.\n");
	}

	printf("done indexing!\n");

exit:
	if (keyval_db) {
		printf("closing key-value DB...\n");
		keyval_db_close(keyval_db);
	}

	if (math_index) {
		printf("closing math index...\n");
		math_index_close(math_index);
	}

	if (term_index) {
		printf("closing term index...\n");
		term_index_close(term_index);
	}

	printf("closing dict...\n");
	text_segment_free();

	if (path)
		free(path);

	return 0;
}
