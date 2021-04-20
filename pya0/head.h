#include <Python.h>

PyMODINIT_FUNC PyInit_pya0();

PyObject *use_fallback_parser(PyObject*, PyObject*);

PyObject *do_lexing(PyObject*, PyObject*, PyObject*);
PyObject *do_parsing(PyObject*, PyObject*, PyObject*);

PyObject *index_open(PyObject*, PyObject*, PyObject*);
PyObject *index_close(PyObject*, PyObject*);
PyObject *index_memcache(PyObject*, PyObject*, PyObject*);
PyObject *index_print_summary(PyObject*, PyObject*);
PyObject *index_lookup_doc(PyObject*, PyObject*);

PyObject *indexer_new(PyObject*, PyObject*);
PyObject *indexer_del(PyObject*, PyObject*);
PyObject *do_maintain(PyObject*, PyObject*, PyObject*);
PyObject *do_flush(PyObject*, PyObject*);
PyObject *add_document(PyObject*, PyObject*, PyObject*);

PyObject *do_search(PyObject*, PyObject*, PyObject*);
