#include "head.h"

static PyMethodDef module_funcs[] = {
	{"lex", do_lexing, METH_VARARGS, "scan tokens"},
	{"index_open", (PyCFunction)index_open, METH_VARARGS | METH_KEYWORDS, "open index"},
	{"index_close", index_close, METH_VARARGS, "close index"},
	{"index_writer", indexer_new, METH_VARARGS, "create an indexer"},
	{"writer_close", indexer_del, METH_VARARGS, "delete an indexer"},
	{"writer_maintain", (PyCFunction)do_maintain, METH_VARARGS | METH_KEYWORDS, "maintain index"},
	{"writer_flush", do_flush, METH_VARARGS, "flush index"},
	{"writer_add_doc", (PyCFunction)add_document, METH_VARARGS | METH_KEYWORDS, "add document"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
	PyModuleDef_HEAD_INIT,
	"pya0",
	"Approach Zero Python Interface",
	-1, module_funcs
};

PyMODINIT_FUNC PyInit_pya0(void)
{
	return PyModule_Create(&module);
}
