#!/bin/sh
if [ $# -lt 1 ]; then
	echo "bad args"
	exit
fi;

query=$(cat research_example/${1}.tex | head -1)
docmt=$(cat research_example/${1}.tex | tail -1)

printf 'Query: %s\n' "$query"
./run/toy.out > lrpath_q.tmp << EOF
$query
EOF

printf 'Doc: %s\n' "$docmt"
./run/toy.out > lrpath_d.tmp << EOF
$docmt
EOF

./sim.py lrpath_q.tmp lrpath_d.tmp
