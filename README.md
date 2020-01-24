This branch is saved for reproducing research paper results we submitted on ECIR 2020.

## Instructions to reproduce results

### 1. Check out documentation to compile the code:
[https://approach0.xyz/docs](https://approach0.xyz/docs)

### 2. Prepare indexing
Use `vdisk-creat.sh` and `vdisk-mount.sh' scripts in `indexer/scripts` to create Reiserfs filesystem on virtual disk which supports unlimited number of files.
(documented [here](https://approach0.xyz/docs/src/vdisk.html))

### 3. Indexing
Start indexd:
```sh
$ cd indexer
$ ./run/indexd.out -o ./mnt-vdisk.img/
```

For [NTCIR corpus](https://drive.google.com/open?id=1emboT7k4m7yKjru3AOb1xScZgbUnQuC8), use `indexer/test-corpus/ntcir12/ntcir-feeder.py` (modify corresponding path in the script) to feed indexd.
For [MSE corpus](https://www.cs.rit.edu/~dprl/data/mse-corpus.tar.gz), use `indexer/scripts/json-feeder.py` to feed indexd.

### 4. Effectiveness evaluation
In order to reproduce Bpref scores, make sure LaTeXML is [installed](https://approach0.xyz/docs/src/appendix_indri.html#install-latexml), even before NTCIR corpus is indexed.

Use `genn-trec-results.py` and `eval-trec-results-?.sh` scripts to input NTCIR queries and evaluate search results.
You will need to modify some file paths in these scripts to point to your downloaded file paths, here are some files you need to [download](https://drive.google.com/open?id=1emboT7k4m7yKjru3AOb1xScZgbUnQuC8) first:

* NTCIR topics file: `topics.txt`
* NTCIR judgement file: `NTCIR12_MathWiki-qrels_judge.dat`
* trec_eval program

Setup search daemon:
```sh
$ cd searchd
$ ./run/searchd.out -i ~/nvme0n1/index-ntcir/ -T -c 0
```
where `-c` specify the cache memory size. `-T` will make searchd output TREC format results.

Feed NTCIR queries:
```
$ ./genn-trec-results.py > all.dat
```

Bpref results can be given by
```sh
$ ./eval-trec-results-summary.sh ./all.dat
Full relevance:
bpref                   all     0.5132
P_5                     all     0.3550
P_10                    all     0.2375
P_15                    all     0.1900
P_20                    all     0.1775
Partial relevance:
bpref                   all     0.4573
P_5                     all     0.4600
P_10                    all     0.3725
P_15                    all     0.3367
P_20                    all     0.3175
```

### 5. Efficiency evaluation
Similar to above steps. In our paper, you will need to record merge time, which can be found in searchd log file `merge.runtime.dat`.
Specify `-c 0` to searchd to execute on-disk runs, and caching all posting lists for in-memory runs. 

Parameters related to this paper can be changed by modifying `search/config.h` file and recompile the project, alternatively, issue
```sh
$ make update
```
under search directory after you have changed the source code configs.

Here are a few interesting parameters related to this paper:
```c
RANK_SET_DEFAULT_VOL
STRATEGY_NONE
STRATEGY_MAXREF
STRATEGY_GBP_NUM
STRATEGY_GBP_LEN
```
Modify/uncomment the macros to change parameters.

Notice: Caching all posting lists into memory consumes a lot of time, use `cache-list.tmp` [file](https://github.com/approach0/search-engine/tree/ecir2019#retrieval) to speedup caching.

### 6. Automation scripts
There are some automation scripts we use to generate our paper results.
`ecir-auto-eval.py` and `ecir-collect.py` are used to run queries with different search parameters and collect efficiency results.
`cnt-invlist-items.sh` and `get-stats.py` under `math-index/script` are used to get inverted lists statistics.
Again, you will need to modify some hard coded paths accordingly in these scripts to be able to run them.

### 7. Implementation
Our source code that implements pseudo code described in the paper can be found at the `math_l2_postlist_next` function in `search/math-search.c`.
Scoring functions are implemented in `math_expr_set_score__opd_only` function in `search/math-expr-sim.c`.

### Questions
Please email wxz8033@g.rit.edu if you have any questions.
