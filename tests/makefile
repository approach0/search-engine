include ../rules.mk

all: run/indexer.ln run/txt2json.ln run/searcher.ln \
	run/search-math-doc-by-url.ln

run:
	mkdir run

run/indexer.ln: run
	ln -sf `pwd`/../indexer/run/indexer.out $@

run/txt2json.ln: run
	ln -sf `pwd`/../indexer/scripts/txt2json.py $@

run/searcher.ln: run
	ln -sf `pwd`/../search/run/test-search.out $@

run/search-math-doc-by-url.ln: run
	ln -sf `pwd`/../search/run/search-math-doc-by-url.out $@

clean:
	rm -rf run
	rm -rf tmp-corpus
