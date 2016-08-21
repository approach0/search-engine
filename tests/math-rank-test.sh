#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Math rank test script to test one test-case.

Examples:
$0 cases/math-rank/inequality.txt
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

# set variable
testcase="$1"

# clear potential old corpus in intermediate directory
itm=./intermediate
rm -rf $itm
mkdir -p $itm

# clear expected-rank file
> $itm/expected-rank

# extract sections
head -1 "$testcase" > $itm/query
tail -n +3 "$testcase" > $itm/docs

# print test query
echo "[test query] $(cat "$itm/query")"

# for each doc in docs
echo "Expected ranking:"
i=0
while read -r line
do
	# parse line
	prefix=$(echo "$line" | awk '{print $1}')
	tex=$(echo "$line" | cut -d' ' -f 2- )

	if [ "$prefix" != 'HIT' -a  "$prefix" != 'NOT' ]; then
		echo "invalid test-case format."
		exit 2
	fi;

	# generate corpus files
	echo "[imath]$tex[/imath]" > $itm/doc${i}
	./txt2json.ln $itm/doc${i} > /dev/null

	# generate expect ranking
	if [ "$prefix" == 'HIT' ]; then
		echo "HIT doc${i}: $tex"
		echo "doc${i}" >> $itm/expected-rank
	else
		echo "NOT doc${i}: $tex"
	fi;

	# increment i
	let 'i=i+1'
done << EOF
$(cat "$itm/docs")
EOF

# index those corpus files
./indexer.ln -e -p $itm > /dev/null

# search test query
qry=$(cat "$itm/query")

# echo full search command
set -x
./searcher.ln -e -n -i ./tmp -m "$qry" > $itm/search-results
set +x

# extract search results
grep 'URL:' $itm/search-results | awk '{print $2}' | \
	awk -F/ '{print $NF}' > $itm/real-rank

echo 'Real ranking:'
cat $itm/real-rank

# compare with expected ranking
diff $itm/expected-rank $itm/real-rank > /dev/null

if [ $? -eq 0 ]; then
	# exact what we expect
	tput setaf 2
	echo "test pass."
	tput sgr0
	exit 0
else
	# different from what we expect
	tput setaf 1
	echo "test fails."
	tput sgr0

	# log failure
	echo "$testcase" >> fail.log

	exit 1
fi;
