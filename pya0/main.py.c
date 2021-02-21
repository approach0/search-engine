#include "head.h"

static PyMethodDef module_funcs[] = {
	{"lex", do_lexing, METH_VARARGS, "scan tokens"},
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
