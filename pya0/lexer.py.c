#include <stdio.h>
#include <stdlib.h>

#include "head.h"
#include "tex-parser/head.h"
#include "tex-parser/y.tab.h"

PyObject *do_lexing(PyObject *self, PyObject *args)
{
	char *string;
	if (!PyArg_ParseTuple(args, "s", &string)) {
		PyErr_Format(PyExc_RuntimeError,
			"Expecting string argument");
		return NULL;
	}

	/* create parser buffer */
	size_t scan_buf_sz;
	char *scan_buf;
	scan_buf = mk_scan_buf(string, &scan_buf_sz);
	YY_BUFFER_STATE state_buf;
	state_buf = yy_scan_buffer(scan_buf, scan_buf_sz);

	/* allocate Python List */
	PyObject *item, *list = PyList_New(0);

	/* scan tokens */
	char *token = NULL, *symbol = NULL;
	struct optr_node* nd;
	int next;
	while ((next = yylex())) {
		nd = yylval.nd;
		if (nd) {
			/* get token and symbol in string */
			token = trans_token(nd->token_id);
			symbol = trans_symbol(nd->symbol_id);
			/* append item */
			item = Py_BuildValue("lss", next, token, symbol);
			PyList_Append(list, item); /* only lend the ref */
			Py_DECREF(item);
			/* release union */
			optr_release(nd);
			yylval.nd = NULL;
		}
	}

	yy_delete_buffer(state_buf);
	free(scan_buf);

	yylex_destroy();
	return list;
}
