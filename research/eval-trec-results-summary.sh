#!/bin/sh
cmd=trec_eval
echo 'Full relevance:'
$cmd -l3 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -v "docno" | grep -E "(^P|bpref)" | head -5
full=`$cmd -l3 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -v "docno" | grep 'bpref' | awk '{print $3}'`
echo 'Partial relevance:'
$cmd -l1 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -v "docno" | grep -E "(^P|bpref)" | head -5
part=`$cmd -l1 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -v "docno" | grep 'bpref' | awk '{print $3}'`

# copy to clipboard and make it easy to paste on spreadsheet.
# echo "${part} \t ${full}" | xclip -selection c
