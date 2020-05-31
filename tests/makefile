include ../rules.mk

all: math-rank-test

math-rank-test: searcher.ln indexer.ln
	@ find cases/math-rank/ -type f \
	-exec echo {} \; -exec ./$@.sh {} \;

indexer.ln:
	ln -sf `pwd`/../indices-v3/run/test-doc-write.out $@

searcher.ln:
	ln -sf `pwd`/../search-v3/run/test-search.out $@

clean:
	rm -rf intermediate
