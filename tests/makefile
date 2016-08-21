include ../rules.mk

all: math-rank-test

math-rank-test: links
	@ find cases/math-rank/ -type f -exec ./$@.sh {} \;

links: indexer.ln txt2json.ln searcher.ln \
	search-math-doc-by-url.ln

indexer.ln:
	ln -sf `pwd`/../indexer/run/indexer.out $@

txt2json.ln:
	ln -sf `pwd`/../indexer/scripts/txt2json.py $@

searcher.ln:
	ln -sf `pwd`/../search/run/test-search.out $@

search-math-doc-by-url.ln:
	ln -sf `pwd`/../search/run/search-math-doc-by-url.out $@

clean:
	rm -rf intermediate
