#include "head.h"

static PyMethodDef module_funcs[] = {
	{"use_fallback_parser", use_fallback_parser, METH_VARARGS, "disable/enable using fallback TeX parser"},
	{"tokenize", (PyCFunction)do_lexing, METH_VARARGS | METH_KEYWORDS, "scan LaTeX and produce tokenized tokens"},
	{"parse", (PyCFunction)do_parsing, METH_VARARGS | METH_KEYWORDS, "parse LaTeX to OPT."},

	{"index_open", (PyCFunction)index_open, METH_VARARGS | METH_KEYWORDS, "open index"},
	{"index_close", index_close, METH_VARARGS, "close index"},
	{"index_memcache", (PyCFunction)index_memcache, METH_VARARGS | METH_KEYWORDS, "cache specified index volume into memory"},
	{"index_print_summary", index_print_summary, METH_VARARGS, "print index summary"},
	{"index_lookup_doc", index_lookup_doc, METH_VARARGS, "lookup document content"},
	{"index_writer", indexer_new, METH_VARARGS, "create an indexer"},

	{"writer_close", indexer_del, METH_VARARGS, "delete an indexer"},
	{"writer_maintain", (PyCFunction)do_maintain, METH_VARARGS | METH_KEYWORDS, "maintain index"},
	{"writer_flush", do_flush, METH_VARARGS, "flush index"},
	{"writer_add_doc", (PyCFunction)add_document, METH_VARARGS | METH_KEYWORDS, "add document"},

	{"search", (PyCFunction)do_search, METH_VARARGS | METH_KEYWORDS, "perform math-aware search"},

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
