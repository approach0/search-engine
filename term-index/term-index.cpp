#include <iostream>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <stdlib.h>

#include "config.h"
#include "term-index.hpp"
#include "term-index.h"

using namespace std;

void *term_index_open(const char *path, enum term_index_open_flag flag)
{
	struct term_index *ti = new struct term_index;
	uint32_t docN, doci, doclen;

	ti->parameters.set("memory", 1024 * 1024 * 512);
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

	/* calculate avgDocLen */
	docN = term_index_get_docN(ti);
	ti->avgDocLen = 0;
	for (doci = 1; doci <= docN; doci++) {
		doclen = ti->index->documentLength(doci);
		if (ti->avgDocLen < UINT32_MAX - doclen)
			ti->avgDocLen += doclen;
		else
			break;
	}
	if (doci > 1)
		ti->avgDocLen = ti->avgDocLen / (doci - 1);
	else
		ti->avgDocLen = 0;
	//printf("=%u\n", ti->avgDocLen);

	/* text index cache, initially empty */
	ti->trp_root = NULL;

	return ti;
}

void term_index_close(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	ti->repo.close();
	delete ti;

	/* free text index cache */
	if (ti->trp_root)
		term_index_cache_free(ti);
}

int term_index_maintain(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	ti->repo.write();
	ti->repo.merge();

	return 0;
}

int term_index_write(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	ti->repo.write();

	return 0;
}

size_t term_index_size(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	indri::collection::Repository::index_state state = ti->repo.indexes();
	return state->size();
}

int term_index_should_maintain(void *handle)
{
	struct term_index *ti = (struct term_index*)handle;
	indri::collection::Repository::index_state state = ti->repo.indexes();

	return state->size() > MAXIMUM_INDEX_COUNT;
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

#ifdef IGNORE_TERM_INDEX
	static doc_id_t new_docID = 0;
	new_docID ++;
#else
	doc_id_t new_docID = ti->repo.addDocument(&ti->document);
#endif

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

void term_posting_finish(void *posting)
{
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	delete po;
}

uint64_t term_posting_cur(void *posting)
{
	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	indri::index::DocListIterator::DocumentData *doc;
	doc = po->currentEntry();

	return (doc == NULL) ? UINT64_MAX : doc->document;
}

size_t term_posting_read(void *posting, void *dest)
{
	struct term_posting_item *p = (struct term_posting_item *)dest;

	indri::index::DocListIterator *po = (indri::index::DocListIterator*)posting;
	indri::index::DocListIterator::DocumentData *doc;

	doc = po->currentEntry();

	if (doc) {
		p->doc_id = doc->document;
		p->tf = doc->positions.size();
		p->n_occur = min(MAX_TERM_ITEM_POSITIONS, (int)p->tf);

		for (unsigned int k = 0; k < p->n_occur; k++) {
			p->pos_arr[k] = doc->positions[k];
		}

		return sizeof(struct term_posting_item);
	} else {
		return 0;
	}
}
