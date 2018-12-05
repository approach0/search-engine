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
* Ubuntu 16.04.3 LTS
* gcc version: 5.4.0 20160609
* Support reiserFS (optional)
	* If reiserFS is not installed, you may try to install the `reiserfsprogs` package in Ubuntu case
	* If you do not want the index image to be partitioned in reiserFS, pass available partition options to `vdisk-creat.sh` script instead.

## Steps to reproduce the results
### (Indexing)
1. Clone the source code and checkout to this branch:
```
$ git clone https://github.com/approach0/search-engine.git
$ git checkout ecir2019
```

2. Build the code according to this documentation:
[https://approach0.xyz/docs](https://approach0.xyz/docs/src/build.html)

3. Create new index image
```sh
$ cd $PROJECT/indexer
$ ./scripts/vdisk-creat.sh reiserfs 2 # create 2 GB new index image partitioned in reiserFS
$ sudo ./scripts/vdisk-mount.sh reiserfs vdisk.img
```
(`$PROJECT` is the environment variable pointing to your cloned repository)

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
(this process is going to take hours even on a SSH drive because our indexer is implemented very naively)

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
Since in our paper we only evaluate non-wildcard queries, you also need to delete those lines in `topic.txt` where it associates to `NTCIR12-MathWiki-21` to `NTCIR12-MathWiki-40`.
You can use this command to do the job:
```sh
$ cat topics.txt | head -20 > topics-concrete.txt
```

Then simply issue
```sh
$ ./genn-trec-results.py > search-results.trec.tmp
```
to generate search results for all 20 queries, output overwrites `search-results.trec.tmp` which contains the search results in TREC format.

### (Evaluation)
8. Evaluate search results
Now you can evaluate the generated results using `trec_eval` command (get it [here](https://github.com/usnistgov/trec_eval)).

For example, for full-relevance scores
```sh
$ trec_eval -l3 /path/to/judge.dat ./search-results.trec.tmp
```

for partial-relevance scores
```sh
$ trec_eval -l1 /path/to/judge.dat ./search-results.trec.tmp
```

Unmount the index device image before you need to delete the index image:
```sh
$ cd $PROJECT/indexer
$ sudo umount ./mnt-vdisk.img
```

## License
MIT

## Contact
wxz8033@rit.edu
