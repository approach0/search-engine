#undef NDEBUG /* make sure assert() works */
#include <assert.h>

#include <Python.h>
#include <stdio.h>
#include <string.h>

#define JIEBA_WRAP_NAME     "jieba-wrap" /* jieba-wrap.py file name */
#define JIEBA_WRAP_INIT_FUN "jieba_wrap_init"
#define JIEBA_WRAP_CUT_FUN  "jieba_wrap_cut"
#define JIEBA_WRAP_ADD_WORD "jieba_wrap_add"

#include "jieba-wrap.h"

static PyObject *jieba_wrap_nm, *jieba_module, *jieba_init_func;

long int pyobj_refcnt(void *obj)
{
	return Py_REFCNT((PyObject *)obj);
}

static void py_memory_leak_demo(void)
{
	int i;
	PyObject *py_leak;

	for (i = 0; i < 10; i++) {
		py_leak = PyLong_FromLong(123);
		/* in contrast, PyUnicode_FromString("foo"); does
		 * not increment ref_cnt. */
		PyObject_Print(py_leak, stdout, 0);
		PRINT_REF_CNT(py_leak);
#if 0
		Py_DECREF(py_leak);
#endif
	}
}

static void *call_a_string_fun(const char* fun_name,
                               const char *utf8_str, size_t bytes)
{
	PyObject *py_str, *fun_arg;
	PyObject *ret = NULL;
	PyObject *cut_fun /* new ref */ =
	          PyObject_GetAttrString(jieba_module, fun_name);

	if (cut_fun == NULL) {
		printf("PyObject_GetAttrString(`%s') fails.\n",
		       JIEBA_WRAP_CUT_FUN);
		return NULL;
	}
	
	assert(1 == PyCallable_Check(cut_fun));

	/* make function argument before calling cut() */
	fun_arg = PyTuple_New(1); /* new ref */
	
	/* PyUnicode_FromStringAndSize() Create a new Unicode object
	 * (`py_str') from the char buffer. */
	py_str = PyUnicode_FromStringAndSize(utf8_str, bytes);

	if (py_str == NULL) {
		printf("PyUnicode_FromStringAndSize() fails.\n");
		return NULL;
	}

	/* py_str ref_cnt is stolen by PyTuple_SetItem(),
	 * thus no need to Py_DECREF(py_str). */
	PyTuple_SetItem(fun_arg, 0 /* arg[0] */, py_str);
	ret = PyObject_CallObject(cut_fun, fun_arg);
	
	// PRINT_REF_CNT(py_str);

	// PRINT_REF_CNT(cut_fun);
	Py_DECREF(cut_fun);

	// PRINT_REF_CNT(fun_arg);
	Py_XDECREF(fun_arg);
	
	return ret;
}

void jieba_add_usr_word(const char *utf8_str, size_t bytes)
{
	PyObject *ret = call_a_string_fun(JIEBA_WRAP_ADD_WORD,
	                                  utf8_str, bytes);
	Py_XDECREF(ret);
}

void *jieba_cut(const char *utf8_str, size_t bytes)
{
	return call_a_string_fun(JIEBA_WRAP_CUT_FUN, utf8_str, bytes);
}

void foreach_tok(void *gen_toks, jieba_token_callbk callfun, void *arg)
{
	PyObject *it = PyObject_GetIter(gen_toks); /* new ref */
	PyObject *tok = NULL;
	PyObject *py_term, *py_begin, *py_end, *py_tag;
	long int begin, end;
	char *term, *tag;

	if (it == NULL) {
		printf("PyObject_GetIter() fails.\n");
		return;
	}

	while (NULL != (tok /* new ref */ = PyIter_Next(it))) {
		assert(PyTuple_Check(tok) == 1);

		/* PyTuple_GetItem() returns borrowed
		 * reference, no need to call Py_DECREF() */
		py_term  = PyTuple_GetItem(tok, 0);
		py_begin = PyTuple_GetItem(tok, 1);
		py_end   = PyTuple_GetItem(tok, 2);
		py_tag   = PyTuple_GetItem(tok, 3);

		/* for PyUnicode_AsUTF8() the caller is not
		 * responsible for deallocating the buffer. */
		term  = PyUnicode_AsUTF8(py_term);
		begin = PyLong_AsLong(py_begin);
		end   = PyLong_AsLong(py_end);
		tag   = PyUnicode_AsUTF8(py_tag);

		callfun(term, begin, end, tag, arg);
		// PRINT_REF_CNT(tok);
		Py_DECREF(tok);
	}

	//PRINT_REF_CNT(it);
	Py_DECREF(it);
}

void jieba_token_print(char *term, long int begin,
                       long int end, char *tag, void *arg)
{
	printf("%s: <%ld, %ld> tag: `%s'\n",
			term, begin, end, tag);
}

static void token_donothing(char *term, long int begin,
                            long int end, char *tag, void *arg)
{
	return;
}

void jieba_release(void)
{
	Py_DECREF(jieba_module);

	printf("calling Py_Finalize()...\n");
	// PRINT_REF_CNT(jieba_module);

	Py_Finalize();
}

void jieba_init(void)
{
	void *res;
	Py_Initialize();

	if (!Py_IsInitialized()) {
		printf("Py_Initialize() fails.\n");
		assert(0);
	}

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('./')");

	/* Decode a string using Py_FileSystemDefaultEncoding */
	jieba_wrap_nm = PyUnicode_DecodeFSDefault(JIEBA_WRAP_NAME);

	jieba_module = PyImport_Import(jieba_wrap_nm); /* new ref */
	if (jieba_module == NULL) {
		printf("PyImport_Import() fails.\n");
		goto free;
	}

	jieba_init_func = PyObject_GetAttrString(jieba_module,
			JIEBA_WRAP_INIT_FUN); /* new ref */
	if (jieba_init_func == NULL) {
		printf("PyObject_GetAttrString() fails.\n");

		jieba_release();
		jieba_module = NULL;
	}

	assert(1 == PyCallable_Check(jieba_init_func));

	/* call jieba_init_func in Python */
	res = PyObject_CallObject(jieba_init_func, NULL); /* new ref */
	Py_DECREF(res);
	Py_DECREF(jieba_init_func);

free:
	/* should free obj returned by PyUnicode_DecodeFSDefault(), see #1.3:
	 * http://python.readthedocs.org/en/latest/extending/embedding.html */
	Py_DECREF(jieba_wrap_nm);

	assert(jieba_module != NULL);

	goto invoke_lazy_dict_load; /* uncomment if you do not want this */
	return;

invoke_lazy_dict_load:
	res = jieba_cut("初始化test", strlen("初始化test"));
	if (res) {
		foreach_tok(res, &token_donothing, NULL);

		// PRINT_REF_CNT(res);
		Py_DECREF(res);
	}
}
