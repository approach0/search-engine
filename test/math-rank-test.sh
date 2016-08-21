#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Math rank test script to test one test-case.

Examples:
$0 cases/inequality.txt
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

# make tmp directory
tmp_corpus=./tmp-corpus
mkdir -p $tmp_corpus

# set variable
testcase="$1"

# clear expect-rank file
> expect-rank.log

# extract sections
head -1 "$testcase" > $tmp_corpus/query
tail -n +3 "$testcase" > $tmp_corpus/docs

# print test query
echo "[test query: $(cat "$tmp_corpus/query")]"

# for each doc in docs
i=0
while read line
do
	# parse line
	prefix=$(echo "$line" | awk '{print $1}')
	tex=$(echo "$line" | cut -d' ' -f 2- )

	if [ "$prefix" != 'HIT' -a  "$prefix" != 'NOT' ]; then
		echo "invalid test-case format."
		exit 2
	fi;

	# generate corpus files
	echo "[imath]$tex[/imath]" > $tmp_corpus/doc${i}
	./run/txt2json.ln $tmp_corpus/doc${i} > /dev/null

	# generate expect ranking
	if [ "$prefix" == 'HIT' ]; then
		echo "expect   doc${i}: $tex"
		echo "doc${i}" >> expect-rank.log
	else
		echo "unexpect doc${i}: $tex"
	fi;

	# increment i
	let 'i=i+1'
done << EOF
$(cat "$tmp_corpus/docs")
EOF

# index those corpus files
./run/indexer.ln -e -p $tmp_corpus > /dev/null

# search test query
qry=$(cat "$tmp_corpus/query")
echo "[searching $qry]"

# echo full search command
set -x
./run/searcher.ln -e -n -i ./tmp -m "$qry" > srch-res.log
set +x

# extract search results
grep 'URL:' srch-res.log | awk '{print $2}' | \
	awk -F/ '{print $NF}' > rank.log

echo '[rank]'
cat rank.log

echo '[diff]'
diff expect-rank.log rank.log > /dev/null

if [ $? -eq 0 ]; then
	# exact what we expect
	echo 'same'
	exit 0
else
	# different from what we expect
	echo 'different'
	exit 1
fi;
