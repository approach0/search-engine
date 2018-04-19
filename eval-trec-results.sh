#!/bin/sh
./trec_eval -l3 NTCIR12_MathWiki-qrels_judge.dat $1
echo 'query specific:'
./trec_eval -q -l3 NTCIR12_MathWiki-qrels_judge.dat $1 | grep bpref
