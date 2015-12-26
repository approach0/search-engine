#!/bin/bash
if [ $# -eq 1 ] && [ $1 == '-h' ]
then
	echo 'DESCRIPTION:'
	echo 'auto-generate discriminative tokens.'
	echo ''
	echo 'SYNOPSIS:'
	echo "$0 <.l file> <symbol header>"
	echo ''
	echo 'OUTPUT:'
	echo "gen-<.l file> gen-<symbol header>"
	exit
fi

[ $# -ne 2 ] && echo "bad arg." && exit
[ ! -e ${1} ] && echo "$1 not exists" && exit
[ ! -e ${2} ] && echo "$2 not exists" && exit

lexer_file="${1}"
sym_header="${2}"
tmpfile=replace-lines.tmp

cp $lexer_file gen-${lexer_file}
cp $sym_header gen-${sym_header}

grep -n 'RET_TOK(NIL' ${lexer_file} | awk '{print $1}' | awk 'BEGIN {FS=":|\\\\"} {print $1, $4 }' > ${tmpfile}

while read line
do 
	num=`echo $line | cut -d' ' -f1`
	sym=`echo $line | cut -d' ' -f2`
	echo "gen symbol table item ${sym}"
	sed -i "${num}s/NIL/${sym}/" gen-${lexer_file}
	sed -i "/INSERT_HERE/a\\\tS_${sym}," gen-${sym_header}
done < ${tmpfile}
