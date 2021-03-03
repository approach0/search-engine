#include <string.h>

#include "common/common.h"
#include "txt-seg/txt-seg.h"
#include "txt-seg/lex.h"
#include "search-v3/search.h"
#include "searchd/json-utils.h"
#include "searchd/trec-res.h"
#include "head.h"

PyObject *do_search(PyObject *self, PyObject *args, PyObject* kwargs)
{
	/* parse arguments */
	PyObject *pyindices, *pylist;
	int verbose = 0, topk = 20;
	const char *trec_output = NULL;
	static char *kwlist[] = {"index", "keywords", "verbose", "topk", "trec_output", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|pis", kwlist,
		&pyindices, &pylist, &verbose, &topk, &trec_output)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTupleAndKeywords error");
		return NULL;
	}

	/* sanity check */
	int list_len = PyObject_Length(pylist);
	if (list_len <= 0) {
		PyErr_Format(PyExc_RuntimeError,
			"Please pass a valid list of keywords as query");
		return NULL;
	}

	/* construct query */
	struct query qry = QUERY_NEW;
	for (int i = 0; i < list_len; i++) {
		PyObject *item = PyList_GetItem(pylist, i);
		if (!PyDict_Check(item))
			PyErr_Format(PyExc_RuntimeError,
				"Query list should contain a dictionary object");

		PyObject *py_kw = PyDict_GetItemString(item, "keyword");
		PyObject *py_type = PyDict_GetItemString(item, "type");

		if (py_kw == NULL || py_type == NULL)
			PyErr_Format(PyExc_RuntimeError,
				"Required key(s) not found in query keyword");

		const char *kw_str = PyUnicode_AsUTF8(py_kw);
		const char *type_str = PyUnicode_AsUTF8(py_type);
		//printf("%s, %s\n", type_str, kw_str); /* for debug */

		if (0 == strcmp(type_str, "term")) {
			query_digest_txt(&qry, kw_str);

		} else if (0 == strcmp(type_str, "tex")) {
			query_push_kw(&qry, kw_str, QUERY_KW_TEX, QUERY_OP_OR);

		} else {
			PyErr_Format(PyExc_RuntimeError,
				"Bad query keyword type");
			return NULL;
		}
	}

	/* print query in verbose mode */
	if (verbose)
		query_print(qry, stdout);

	/* actually perform search */
	struct indices *indices = PyLong_AsVoidPtr(pyindices);
	ranked_results_t srch_res; /* search results */
	if (verbose) {
		srch_res = indices_run_query(indices, &qry, topk, NULL, 0, stdout);
	} else {
		FILE *log_fh = fopen("/dev/null", "a");
		srch_res = indices_run_query(indices, &qry, topk, NULL, 0, log_fh);
		fclose(log_fh);
	}

	/* output TREC format file? */
	if (trec_output) {
		search_results_trec_log(indices, &srch_res, trec_output);
	}

	/* convert search results to JSON stringified */
	const char *ret = search_results_json(&srch_res, -1, indices);

	/* release resources */
	free_ranked_results(&srch_res);
	query_delete(qry);
	return PyUnicode_FromString(ret);
}
