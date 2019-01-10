#!/bin/bash

dir1=/home/tk/Desktop/search-engine-old/tmp
dir2=/home/tk/Desktop/srv-fix-eval/tmp
outdir=/home/tk/Desktop/approach-trec/trecfiles

function eval_cmd()
{
	trec_eval -l1 -q ~/Desktop/ntcir-12/NTCIR12_MathWiki-qrels_judge.dat $1 | grep bpref | sort -V
}

rm -f $outdir/a/*
rm -f $outdir/b/*

for file in `ls $dir1`; do
	echo $file | grep '\.dat$' && true
	if [[ $? != 0 ]]; then
		continue
	fi
	ls $dir1/$file
	ls $dir2/$file
	eval_cmd $dir1/$file > a.tmp
	eval_cmd $dir2/$file > b.tmp
	diff a.tmp b.tmp
	if [[ $? != 0 ]]; then
		cp $dir1/$file $outdir/a/
		cp $dir2/$file $outdir/b/
	fi
done
