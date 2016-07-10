#include "indri/Repository.hpp"
#include "indri/Parameters.hpp"
#include "indri/ParsedDocument.hpp"
#include "indri/CompressedCollection.hpp"
#include "term-index.h"
#include <iostream>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <stdlib.h>

using namespace std;

struct term_index {
	indri::collection::Repository  repo;
	indri::api::Parameters         parameters;
	indri::api::ParsedDocument     document;
	indri::index::Index           *index;
	uint32_t                       avgDocLen;
	vector<char*>                  save;
};

void *term_index_open(const char *path, enum term_index_open_flag flag)
{
	struct term_index *ti = new struct term_index;
	uint32_t docN, doci, doclen;

	ti->parameters.set("memory", "1024K");
	//ti->parameters.set("stemmer.name", "krovertz");
	//cout<< "memory limit:" << ti->parameters.get("memory", "unset") <<endl;
	
	if (flag == TERM_INDEX_OPEN_CREATE) {
		if (indri::collection::Repository::exists(path)) {
			ti->repo.open(path, &ti->parameters);
		} else {
			ti->repo.create(path, &ti->parameters);
		}
	} else if (flag == TERM_INDEX_OPEN_EXISTS) {
		if (indri::collection::Repository::exists(path)) {
			ti->repo.open(path, &ti->parameters);
		} else {
			return NULL;
		}
	} else {
		return NULL;
	}

	ti->index = (*ti->repo.indexes())[0];
	ti->document.terms.clear();
	ti->document.positions.clear();
	ti->document.tags.clear();
	ti->document.metadata.clear();
	ti->document.text = NULL;
	ti->document.textLength = 0;
	ti->document.content = NULL;
	ti->document.contentLength = 0;

	/* update avgDocLen */
	//cout<< "calculating avgDocLen..." << endl;
	docN = term_index_get_docN(ti);
	ti->avgDocLen = 0;
	for (doci = 1; doci <= docN; doci++) {
		doclen = ti->index->documentLength(doci);
		if (ti->avgDocLen < UINT32_MAX - doclen)
			ti->avgDocLen += doclen;
		else
			break;
	}
	//printf("avgDocLen=%u/%u", ti->avgDocLen, doci - 1);
	if (doci > 1)
		ti->avgDocLen = ti->avgDocLen / (doci - 1);
	else
		ti->avgDocLen = 0;

	//printf("=%u\n", ti->avgDocLen);

	return ti;
}

void term_index_close(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	ti->repo.close();
	delete ti;

}

int term_index_maintain(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	indri::collection::Repository::index_state state = ti->repo.indexes();
	bool should_merge = state->size() > 50; //3;

	if (should_merge) {
		ti->repo.write();
		ti->repo.merge();
		return 1;
	}

	return 0;
}

void term_index_doc_begin(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	ti->document.terms.clear();

	ti->save.clear();
}

void term_index_doc_add(void *handle, char *term)
{
	struct term_index *ti = (struct term_index*)handle;
	char *p = strdup(term);

	ti->document.terms.push_back(p);

	//printf("%s address: %p\n", p, p);
	ti->save.push_back(p);
}

doc_id_t term_index_doc_end(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	doc_id_t new_docID = ti->repo.addDocument(&ti->document);

	vector<char*>::iterator it;
	vector<char*> &terms = ti->save;
	for (it = terms.begin(); it != terms.end(); it++) {
		//printf("free %s address: %p\n", *it, *it);
		free(*it);
	}

	return new_docID;
}

uint32_t term_index_get_termN(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->index->uniqueTermCount();
}

uint32_t term_index_get_docN(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->index->documentCount();
}

uint32_t term_index_get_docLen(void *handle, doc_id_t doc_id)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->index->documentLength(doc_id);
}

uint32_t term_index_get_avgDocLen(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->avgDocLen;
}

uint32_t term_index_get_df(void *handle, term_id_t term_id)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->index->documentCount(ti->index->term(term_id));
}

term_id_t term_lookup(void *handle, char *term)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->index->term(term);
}

char *term_lookup_r(void *handle, term_id_t term_id)
{
	struct term_index *ti = (struct term_index*)handle;
	return strdup(ti->index->term(term_id).c_str());
}

void *term_index_get_posting(void *handle, term_id_t term_id)
{
	struct term_index *ti = (struct term_index*)handle;
	return ti->index->docListIterator(term_id);
}

bool term_posting_start(void *posting)
{
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;

	po->startIteration();
	return (0 == po->finished());
}

/* returns false if pass the last posting item. */
bool term_posting_next(void *posting)
{
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	return po->nextEntry();
}

/* find the first document which contains an ID >= given ID.
 * returns false if no such document exists. */
bool term_posting_jump(void *posting, uint64_t doc_id)
{
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	return po->nextEntry(doc_id);
}

#include "searchd/config.h" /* for MAX_MERGE_POSTINGS */

struct term_posting_item *term_posting_current(void *posting)
{
	static struct term_posting_item *ret, q[MAX_MERGE_POSTINGS];
	static int i = 0;

	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	indri::index::DocListIterator::DocumentData *doc;
	doc = po->currentEntry();

	if (doc) {
		/* a hacky way to return term_posting_item pointers. This array is
		 * made of MAX_MERGE_POSTINGS objects to avoid different posting
		 * lists referring to the same object at merge time, which leads to
		 * all query terms sharing an identical term-frequency, and a wrong
		 * score as a result. */
		ret = &q[i];
		i = (i + 1) % MAX_MERGE_POSTINGS;
		ret->doc_id = doc->document;
		ret->tf = doc->positions.size();

		return ret;
	} else {
		return NULL;
	}
}

void term_posting_finish(void *posting)
{
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	delete po;
}

position_t *term_posting_current_termpos(void *posting)
{
	uint32_t tf, i;
	position_t *pos_arr;
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	indri::index::DocListIterator::DocumentData *doc;
	indri::utility::greedy_vector<int>::iterator it;

	po = (indri::index::DocListIterator*)posting;
	doc = po->currentEntry();
	tf = doc->positions.size();

	pos_arr = (position_t*)malloc(sizeof(position_t) * tf);

	for (i = 0, it = doc->positions.begin();
	     it != doc->positions.end();
	     i++, it++) {
		pos_arr[i] = *it;
	}

	return pos_arr;
}
