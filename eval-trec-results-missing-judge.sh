#!/bin/sh
cmd=trec_eval
url=http://localhost:3838/get
low_uri=lowerbound/judge.dat
upp_uri=upperbound/judge.dat

file_path=$1
run_name=$2

rm -f ./judge.dat
wget $url/$run_name/$upp_uri

echo 'Full relevance (lower bound): '
$cmd -l3 NTCIR12_MathWiki-qrels_judge.dat $file_path | grep -E "^P" | head -4

echo 'Full relevance (Condensed): '
$cmd -J -l3 NTCIR12_MathWiki-qrels_judge.dat $file_path | grep -E "^P" | head -4

echo 'Full relevance (upper bound): '
$cmd -l3 ./judge.dat $file_path | grep -E "^P" | head -4

echo '==='

echo 'Partial relevance (lower bound): '
$cmd -l1 NTCIR12_MathWiki-qrels_judge.dat $file_path | grep -E "^P" | head -4

echo 'Partial relevance (Condensed): '
$cmd -J -l1 NTCIR12_MathWiki-qrels_judge.dat $file_path | grep -E "^P" | head -4

echo 'Partial relevance (upper bound): '
$cmd -l1 ./judge.dat $file_path | grep -E "^P" | head -4
