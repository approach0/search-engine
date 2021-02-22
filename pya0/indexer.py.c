#include <string.h>

#include "txt-seg/txt-seg.h"
#include "txt-seg/lex.h"
#include "indices-v3/indices.h"
#include "head.h"

PyObject *index_open(PyObject *self, PyObject *args, PyObject* kwargs)
{
	const char *path, *option = NULL, *seg_dict = NULL;
	static char *kwlist[] = {"path", "option", "segment_dict", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ss", kwlist,
		&path, &option, &seg_dict)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTupleAndKeywords error");
		return NULL;
	}

	int failed;
	struct indices *indices = malloc(sizeof *indices);
	if (NULL == option || NULL != strstr(option, "w")) {
		failed = indices_open(indices, path, INDICES_OPEN_RW);
	} else {
		failed = indices_open(indices, path, INDICES_OPEN_RD);
	}

	if (failed) {
		free(indices);
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (seg_dict)
		text_segment_init(seg_dict);

	return PyLong_FromVoidPtr(indices);
}

PyObject *index_close(PyObject *self, PyObject *args)
{
	PyObject *pyindices;
	if (!PyArg_ParseTuple(args, "O", &pyindices)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTuple error");
		return NULL;
	}

	struct indices *indices = PyLong_AsVoidPtr(pyindices);
	indices_close(indices);
	free(indices);
	text_segment_free();

	Py_RETURN_NONE;
}

static int
parser_exception(struct indexer *indexer, const char *tex, char *msg)
{
	fprintf(stderr, "Parser error: %s in `%s'\n", msg, tex);
	return 0;
}

PyObject *indexer_new(PyObject *self, PyObject *args)
{
	PyObject *pyindices;
	if (!PyArg_ParseTuple(args, "O", &pyindices)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTuple error");
		return NULL;
	}

	struct indices *indices = PyLong_AsVoidPtr(pyindices);
	struct indexer *indexer;
	indexer = indexer_alloc(indices, INDICES_TXT_LEXER, parser_exception);

	return PyLong_FromVoidPtr(indexer);
}

PyObject *indexer_del(PyObject *self, PyObject *args)
{
	PyObject *pyindexer;
	if (!PyArg_ParseTuple(args, "O", &pyindexer)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTuple error");
		return NULL;
	}

	struct indexer *indexer = PyLong_AsVoidPtr(pyindexer);
	indexer_free(indexer);

	Py_RETURN_NONE;
}

PyObject *do_maintain(PyObject *self, PyObject *args, PyObject* kwargs)
{
	PyObject *pyindexer;
	int force = 0;
	static char *kwlist[] = {"writer", "force", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|p", kwlist,
		&pyindexer, &force)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTupleAndKeywords error");
		return NULL;
	}

	struct indexer *indexer = PyLong_AsVoidPtr(pyindexer);
	if (indexer_should_maintain(indexer) || force) {
		indexer_maintain(indexer);
		return PyBool_FromLong(1);
	} else {
		return PyBool_FromLong(0);
	}
}

PyObject *do_flush(PyObject *self, PyObject *args)
{
	PyObject *pyindexer;
	if (!PyArg_ParseTuple(args, "O", &pyindexer)) {
		PyErr_Format(PyExc_RuntimeError,
			"PyArg_ParseTuple error");
		return NULL;
	}

	struct indexer *indexer = PyLong_AsVoidPtr(pyindexer);
	indexer_flush(indexer);

	Py_RETURN_NONE;
}
