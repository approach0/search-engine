#!/bin/bash
OUTFILE=arq2020-task1-a0-submission.tsv
> $OUTFILE
for i in {1..100}; do
	echo "Query ${i}"
	target=../arq-math-task1-queries/${i}.json
	if [ -e $target ]; then
		./test-query.sh "http://localhost:8921/search" $target
	fi
	#cat ../trec-format-results.tmp | awk -v OFS="\t" '{print $1, $3, $4, $5, $6}' | sed "s/_QRY_ID_/A.${i}/g" >> $OUTFILE
done
