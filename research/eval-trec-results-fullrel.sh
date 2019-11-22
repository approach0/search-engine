#!/bin/sh
cmd=trec_eval
echo 'Full relevance:'
$cmd -l3 NTCIR12_MathWiki-qrels_judge.dat $1 | grep -v "docno" | grep -E "(^P|bpref)"
echo 'query specific:'
$cmd -q -l3 NTCIR12_MathWiki-qrels_judge.dat $1 | sort -n | grep -E "(bpref|docno)" | sort -V
