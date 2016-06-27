![](https://travis-ci.org/t-k-/the-day-after-tomorrow.svg)
[![GitHub release](https://img.shields.io/github/release/t-k-/the-day-after-tomorrow.svg?maxAge=2592000)]()

## About
This is a repo for a future math-aware search engine.

No worry, "the day after tomorrow" is a temporary name, will come up a more catchy name for this project someday.

This project is originated from my previous [math-only search engine prototype](https://github.com/t-k-/opmes).
There are some amazing ideas coming up since then, so a decision is made to rewrite most the code.
This time, however, it has the ambition to become a production-level search engine with improved efficiency, and most importantly, to be able to handle queries with both math expressions and general text terms. In another word, to be "math-aware".

This project is still under early development so there is not really much to show here.
But keep your eyes open and pull request is appreciated!

The meaning behind "the day after tomorrow"? Because reinventing a search engine in C has a long way to go, but as the saying goes:

> Today is hard, tomorrow will be worse,
> but the day after tomorrow will be sunshine.
>
> (Jack Ma)

## Quick start

### 0. Clone this project source code:
```
$ git clone --depth=1 https://github.com/t-k-/the-day-after-tomorrow.git
```
The following instructions assume you have cloned this project in to directory `$PROJECT`.

### 1. Install dependencies
Other than commonly system build-in libraries (pthread, libz, libm, libstdc++), ther are some external dependencies you may need to download and install to your system environment:

* [bison](http://ftp.gnu.org/gnu/bison/bison-3.0.tar.xz)
* [flex and libfl](http://sourceforge.net/projects/flex/files/flex-2.5.39.tar.xz/download)
* [libtokyocabinet](http://fallabs.com/tokyocabinet/tokyocabinet-1.4.48.tar.gz)
* [libz](http://zlib.net/zlib-1.2.8.tar.gz)
* [libbz](http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz)
* [Lemur/Indri](https://sourceforge.net/projects/lemur/files/lemur/indri-5.9/indri-5.9.tar.gz/download)
* [libpython3](https://www.python.org/ftp/python/3.5.1/Python-3.5.1.tar.xz)
* [jieba](https://github.com/fxsjy/jieba/archive/v0.36.tar.gz)

For Debian/Ubuntu users, you can instead type the following commands to automatically install above dependencies:
```
$ sudo apt-get update
$ sudo apt-get install bison flex python3-pip python3-dev \
$                      libtokyocabinet-dev libbz2-dev libz-dev
$ sudo pip3 install jieba
```
Lemur/Indri is not likely to be in your distribution's official software repository, so you may need to build and manually specify its library path (see the next step).

Lemur/Indri library is an important dependency for this project, currently this project relies on it to provide full-text index functionality (so that we avoid reinventing the wheel, and we can focus on math search implementation. To combine two search engines, simply merge their results and weight their scores accordingly).


After downloading Indri tarball (indri-5.9 for example), build its libraries:

```
$ (cd indri-5.9 && chmod +x configure && ./configure && make)
```

If Indri reports `undefined reference to ...` when building/linking, install that library **and** rerun configure again:

> After installing the zlib-devel package you must rerun configure
> so that it correctly finds it and adds the library to the ld command.
> (see https://sourceforge.net/p/lemur/discussion/546028/thread/e67752b2)

### 2. Configure dependency path
Our project uses `dep-*.mk` files to configure most dependency paths (or CFLAGS and LDFLAGS). If you have installed above dependency libraries in your system environment, chances are you can just leave these `dep-*.mk` files untouched.

One dependency path you probably have to specify manually is the Lemur/Indri library. If you have downloaded and compiled Lemur/Indri source code at `~/indri-5.9`, type:

```
$ ./configure --indri-path=~/indri-5.9
```
to setup build configuration. This `configure` script also checks necessary libraries for building. If `configure` outputs any library that can not be located by the linker, you may need to double check and install the missing dependency before build.

### 3. Build
Type `make` at project top level (i.e. `$PROJECT`) will do the job.

### 4. Test some commands you build
This project is still in its early stage, nothing really to show you now. However, you can play with some existing commands:

* Run our TeX parser to see the corresponding operator tree of a math expression

	```
	$ ./tex-parser/run/test-tex-parser.out
	edit: a+b/c
	return code:SUCC
	Operator tree:
	    └──(plus) 2 son(s), token=ADD, path_id=1, ge_hash=0000, fr_hash=c301.
	        │──[`a'] token=VAR, path_id=1, ge_hash=c401, fr_hash=8703.
	        └──(frac) 2 son(s), token=FRAC, path_id=2, ge_hash=05f7, fr_hash=7203.
	               │──#1[`b'] token=VAR, path_id=2, ge_hash=c501, fr_hash=7403.
	               └──#2[`c'] token=VAR, path_id=3, ge_hash=c601, fr_hash=7503.

	Subpaths (leaf-root paths/total subpaths = 3/4):
	* VAR(0)/ADD(2)/[path_id=1: type=normal, leaf symbol=`a', fr_hash=8703]
	* VAR(0)/rank1(1)/FRAC(2)/ADD(2)/[path_id=2: type=normal, leaf symbol=`b', fr_hash=7403]
	* FRAC(2)/ADD(2)/[path_id=2: type=gener, ge_hash=05f7, fr_hash=7203]
	* VAR(0)/rank2(1)/FRAC(2)/ADD(2)/[path_id=3: type=normal, leaf symbol=`c', fr_hash=7503]
	```

* Index a corpus/collection and see its index statistics

	1. `$PROJECT/indexer/test-doc` includes a mini test corpus. Optionally, you are suggested to download a slightly larger plain text corpus (e.g. *Reuters-21578* and *Ohsumed* from [University of Trento CATEGORIZATION CORPORA](http://disi.unitn.it/moschitti/corpora.htm)) for performance evaluation. For non-trivial (reasonable large) corpus, you will have the chance to observe the index merging precess under default generated index directory (`$PROJECT/indexer/tmp`).
	2. `cd $PROJECT/indexer` and run `run/test-txt-indexer.out -p ./test-doc` to index corpus files recursively from our mini test corpus directory. 
	3. run `../term-index/run/test-read.out -s -p $PROJECT/indexer/tmp` to take a peek at the index (termN, docN, avgDocLen etc.) you just build. (Pass `-h` argument to see more options for `test-read.out` program)

* Test searching

	By far, there are two search program you can play with: One is term search (or fulltext search) which uses Okapi BM25 scoring method and highlight the keywords in result directly in terminal. To perform fulltext search, pass `-p` option to speficy the index path you want to search on, e.g. for AND merge:
	```
	$ $PROJECT/searchd/run/test-term-search.out -p ./tmp -t 'hacker' -t 'news' -o AND
	```
	for OR merge:
	```
	$ $PROJECT/searchd/run/test-term-search.out -p ./tmp -t 'hello' -t 'nick' -t 'wilde' -o OR
	```
	Alternatively, you can have an experience of math search by running:
	```
	$ $PROJECT/searchd/run/test-math-search.out -p ./tmp -t 'a+b'
	```
	For single TeX query, math search will always perform AND merge.

* Test cached index

	`Searchd` can cache a portion of term index posting lists into memory, the number of posting lists cached is depend on the in-memory cache limit specified (currently just 32MB). To test search results from mixed term index (on-disk and in-memory), run:
	```
	$ cd $PROJECT/searchd
	$ ./run/test-search.out -p ./tmp -t 'window' -t 'period' -t 'old' -t 'coat'
	```

## Module dependencies
![module dependency](https://raw.githubusercontent.com/t-k-/cowpie-lab/master/dep.png)
(boxes are external dependencies, circles are internal modules)

To generate this module dependency graph, issue commands below at the project top directory:

```
$ mkdir -p tmp
$ python3 proj-dep.py --targets > targets.mk
$ python3 proj-dep.py --dot > tmp/dep.dot
$ dot -Tpng tmp/dep.dot > tmp/dep.png
```

## License
MIT

## Contributing
Whoever wants an efficient math-aware search to be reality and interested in making digital math-related content (e.g. a math Q&A website) searchable is invited to contribute in this project.
Start your contributing by writing an issue, contribute a line of code, or a typo fix.
