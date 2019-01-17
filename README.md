## About
This branch saves the source code snapshot for ECIR2019 paper.

## Results
Resources can be downloaded from [http://approach0.xyz:3838/](http://approach0.xyz:3838/), it includes:
* Search results from our system (with tree graph and match highlight, see [example](http://approach0.xyz:3838/3-beta-70-4/NTCIR12-MathWiki-20/Fisher_transformation:0/highlight))
* Search results from MCAT and Tangent-S system
* corpus.txt (contains all document formulas separated by lines)
* topics.txt (contains all NTCIR-12 query set, including wildcard queries)
* judge.dat (contains judgement, relevance rating)
* Evaluation files (contain runtime data, evaluation results and raw search result files in TREC format)

To reproduce our results, you need to at least download `corpus.txt`, `topics.txt` and `judge.dat` files.

## Environment
* Ubuntu 18.04.1 LTS
* gcc version: 7.3.0
* Support reiserFS (optional)
	* If reiserFS is not installed, you may try to install the `reiserfsprogs` package in Ubuntu case
	* If you do not want the index image to be partitioned in reiserFS, pass other available partition options to `vdisk-creat.sh` script instead, or index directly on host filesystem (not recommended, top directory deletion will take a very long time).

## Steps to reproduce the results
### (Indexing)
1. Clone the source code and checkout to this branch:
```
$ git clone https://github.com/approach0/search-engine.git
$ git checkout ecir2019
```

2. Build the code according to this documentation:

[https://approach0.xyz/docs](https://approach0.xyz/docs/src/build.html)

(for reproducing results, our chosen LaTeXML version is 0.8.3; revision 22db863)

3. Create new index image (`$PROJECT` is the environment variable pointing to your cloned repository)
```sh
$ cd $PROJECT/indexer
$ ./scripts/vdisk-creat.sh reiserfs 2 # create 2 GB new index image partitioned in reiserFS
$ sudo ./scripts/vdisk-mount.sh reiserfs vdisk.img
```
you may need to install `reiserfsprogs` to have your system support reiserFS partition, or you can replace `reiserfs` above with `btrfs` for alternative index partition format. However, we recommand `reiserfs` because it performs well with large number of small files (fit in our case where we have many posting list files in a deep directory hierarchy).

4. Run indexer daemon and specify the output index to be our newly created image
```sh
$ ./run/indexd.out -o ./mnt-vdisk.img/
```

5. Indexing NTCIR-12 document formulas

Open another shell, feed the downloaded corpus data to indexer
```sh
$ ln -sf /path/to/corpus.txt $PROJECT/indexer/test-corpus/ntcir12/ntcir12-full.tmp
$ cd $PROJECT/indexer/test-corpus/ntcir12
$ ./ntcir-feeder.py
``` 
(this process is going to take hours even on a SSD drive because our indexer is implemented very naively)

### (Retrieval)
6. Start search daemon
```sh
$ cd $PROJECT/searchd
$ ./run/searchd.out -i ../indexer/mnt-vdisk.img/ -c 0 -T
```
Notice: Caching the entire index into memory takes very long time, so we provide a magic file called `cache-list.tmp` in advance (in `searchd` folder), it contains all path set that NTCIR-12 queries contain, and search daemon will read (if any) `cache-list.tmp` file and cache those paths specified in the file.
If you want to disable caching, just rename `cache-list.tmp` to some other name.

7. Feed NTCIR-12 queries to search daemon

After search daemon is on, you can test arbitrary query by enter formula in a locally hosted WEB front end. Refer to [this documentation](https://approach0.xyz/docs/src/demo.html#install-and-config-nginx-php) to setup the demo WEB page, the demo WEB page will be available locally at `http://localhost/demo`.

To evaluate NTCIR-12 queries, follow the steps below.

At `$PROJECT` directory, modify `genn-trec-results.py` script and change Python variable `fname` to your downloaded `topic.txt` path.
Since in our paper we only evaluate non-wildcard queries (also, we do not support wildcard query yet), you also need to delete those lines associating to `NTCIR12-MathWiki-21` through `NTCIR12-MathWiki-40` in `topic.txt`.
You can use the following command to achieve this:
```sh
$ cat topics.txt | head -20 > topics-concrete.txt
```
So the final `genn-trec-results.py` will contain something like:
```py
...
fname = "/path/to/topics-concrete.txt"
output = 'searchd/trec-format-results.tmp'
...
```

Then simply issue
```sh
$ ./genn-trec-results.py > search-results.trec.tmp
```
to generate search results for all 20 queries, output overwrites `search-results.trec.tmp` which will contain the search results in TREC format.

### (Evaluation)
8. Evaluate search results

Now you can evaluate the generated results using `trec_eval` command (if you do not have it, get it [here](https://github.com/usnistgov/trec_eval)).

For example, for full-relevance scores
```sh
$ trec_eval -l3 /path/to/judge.dat ./search-results.trec.tmp
```

for partial-relevance scores
```sh
$ trec_eval -l1 /path/to/judge.dat ./search-results.trec.tmp
```

9. Reproduce results

Our explored parameters are stored in `./test.tsv` file. Each row (except the first header row) contains a unique set of parameters. To view it, open this file in Google spreadsheet.

We provide an auto-evaluation script `ecir-auto-eval.py` which will replace source code and rebuild with different parameters, through rows of parameters specified in `./test.tsv`.

Ensure `index_dir` variable specified in `ecir-auto-eval.py` correctly points to your index directory (unless you did not follow above steps strictly, you do not need to change it), and make a symbolic link named `NTCIR12_MathWiki-qrels_judge.dat` pointing to your downloaded judge file.
```sh
 ln -sf /path/to/judge.dat $PROJECT/NTCIR12_MathWiki-qrels_judge.dat
```
Then simply run the `ecir-auto-eval.py` to reproduce results (at this point please make sure no search daemon are running, this script will spawn its own search daemon). Results of this script are generated under `./tmp` folder.

To view results generated from `ecir-auto-eval.py`, run
```sh
./ecir-collect.py > test.out.tsv
```
and again, use Google spreadsheet to view generated results.

Our full grid-search evaluation results can be viewed here:
https://docs.google.com/spreadsheets/d/1TQ4Ixj3ock8i-EDqfRiy0KJ4SCB3VJP1VaYC7sy64B4/edit?usp=sharing

### (Finishing)
Unmount the index device image before you need to delete the index image:
```sh
$ cd $PROJECT/indexer
$ sudo umount ./mnt-vdisk.img
```

## License
MIT

## Contact
wxz8033@rit.edu
