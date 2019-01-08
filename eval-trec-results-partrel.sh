#!/bin/sh
cmd=trec_eval
echo 'Partial relevance:'
$cmd -l1 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -v "docno" | grep -E "(^P|bpref)"
echo 'query specific:'
$cmd -q -l1 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -E "(bpref|docno)" | sort -V
