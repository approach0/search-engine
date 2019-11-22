#!/bin/sh
INDEXD_BIN=~/Desktop/search-engine-master/indexer/run/indexd.out
CORPUS=/home/tk/corpus/corpus-mse/
INDEX_ROOT=~/nvme0n1/
INDEX_FEED=~/Desktop/search-engine-master/indexer/scripts/json-feeder.py

function index_mse() {
	rank=$1
	begin=$2
	endat=$3
	port=$((8934 + $rank))
	echo "rank#${rank} from $begin to $endat ..."
	indexd_dir=$(dirname $INDEXD_BIN)
	tmux new -d -s indexd-$rank "cd ${indexd_dir} && ./indexd.out -o ${INDEX_ROOT}/mnt-index-${rank}.img/ \
		-p $port -u /index-${rank} 2> /dev/null"
	tmux new -d -s feeder-$rank "python $INDEX_FEED --corpus-path $CORPUS --begin-from $begin --end-at $endat \
		--indexd-url http://localhost:${port}/index-${rank}"
}

index_mse 0        1 264362
index_mse 1   264363 528726
index_mse 2   528727 793088
index_mse 3   793089 1057449

# monitor the progress
while true; do
	clear;
	for i in 0 1 2 3;
		do echo "$i: $(tmux capture-pane -pt feeder-$i -E 2)"
		echo "";
	done
	sleep 1;
done
