#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

#include "tex-parser/head.h"
#include "tex-parser/y.tab.h"

static PyObject *texlex_open(PyObject *self, PyObject *args)
{
	const char *file_path;
	if (!PyArg_ParseTuple(args, "s", &file_path))
		return NULL;

	FILE *fh = fopen(file_path, "r");

	if (fh == NULL) {
		PyErr_Format(PyExc_RuntimeError,
			"cannot open file %s.", file_path);
		return NULL;
	}

	yyin = fh;
	PyObject_SetAttrString(self, "fh", PyLong_FromVoidPtr(fh));

	return PyLong_FromVoidPtr(fh);
}

static PyObject* texlex_close(PyObject *self, PyObject *args)
{
	PyObject* fh_ = PyObject_GetAttrString(self, "fh");
	FILE *fh = PyLong_AsVoidPtr(fh_);

	yylex_destroy();
	fclose(fh);

	Py_RETURN_NONE;
}

static PyObject* texlex_next(PyObject *self, PyObject *args)
{
	int next = yylex();

	char *opt_token = NULL, *opt_symbol = NULL;
	struct optr_node* nd = yylval.nd;

	if (nd) {
		opt_token = trans_token(nd->token_id);
		opt_symbol = trans_symbol(nd->symbol_id);
		optr_release(nd);
		yylval.nd = NULL;
	}

	return Py_BuildValue("lss", next, opt_token, opt_symbol);
}

static PyObject* texlex_trans(PyObject *self, PyObject *args)
{
	if (0 == strcmp(yytext, "\n")) {
		return PyUnicode_DecodeUTF8("\\n", 1, "decode-utf8");
	} else {
		return PyUnicode_DecodeUTF8(yytext, strlen(yytext), "decode-utf8");
	}
}

static PyMethodDef texlex_funcs[] = {
	{"open", texlex_open, METH_VARARGS, "open file"},
	{"next", texlex_next, METH_NOARGS, "scan a token"},
	{"trans", texlex_trans, METH_NOARGS, "translate token"},
	{"close", texlex_close, METH_NOARGS, "close file"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef texlex_module = {
	PyModuleDef_HEAD_INIT,
	"texlex",
	"simple tex lexer",
	-1, texlex_funcs
};

PyMODINIT_FUNC PyInit_texlex(void)
{
	return PyModule_Create(&texlex_module);
}
