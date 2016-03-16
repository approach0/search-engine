#!/bin/bash
if [ $# -eq 1 ] && [ $1 == '-h' ]
then
	echo 'DESCRIPTION:'
	echo 'auto-generate discriminative tokens.'
	echo ''
	echo 'SYNOPSIS:'
	echo "$0 <.l file> <symbol header> <generated lexer output> <header output>"
	echo ''
	echo 'OUTPUT:'
	echo "<generated lexer output> <header output>"
	exit
fi

[ $# -ne 4 ] && echo "bad arg." && exit
[ ! -e ${1} ] && echo "$1 not exists" && exit
[ ! -e ${2} ] && echo "$2 not exists" && exit

lexer_file="${1}"
sym_header="${2}"
output_lexer="${3}"
output_header="${4}"
tmpfile=replace-lines.tmp

cp $lexer_file ${output_lexer}
cp $sym_header ${output_header}

grep -n 'RET_TOK(NIL' ${lexer_file} | awk '{print $1}' | awk 'BEGIN {FS=":|\\\\"} {print $1, $4 }' > ${tmpfile}

while read line
do 
	num=`echo $line | cut -d' ' -f1`
	sym=`echo $line | cut -d' ' -f2`
	echo "gen symbol table item ${sym}"
	sed -i "${num}s/NIL/${sym}/" ${output_lexer}
	sed -i "/INSERT_HERE/a\\\tS_${sym}," ${output_header}
done < ${tmpfile}
