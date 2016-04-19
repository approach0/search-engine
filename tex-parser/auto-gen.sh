#!/bin/bash
if [ $# -eq 1 ] && [ $1 == '-h' ]
then
	echo 'DESCRIPTION:'
	echo 'auto-generate output files from template files.'
	echo ''
	echo 'SYNOPSIS:'
	echo "$0 <template .l> <template symbol .h> <template token .h>"
	echo ''
	echo 'OUTPUT:'
	echo "gen-<template .l> gen-symbol.h gen-token.h"
	exit
fi

[ $# -ne 3 ] && echo "bad arg." && exit
[ ! -e ${1} ] && echo "$1 not exists" && exit
[ ! -e ${2} ] && echo "$2 not exists" && exit
[ ! -e ${3} ] && echo "$3 not exists" && exit

templ_l="${1}"
templ_sym_h="${2}"
templ_tok_h="${3}"
out_l=gen-lexer.l
out_sym_h=gen-symbol.h
out_tok_h=gen-token.h

tmpfile=auto-gen.tmp
auto_macro=_AUTOGEN_

cp ${templ_l} ${out_l}
cp ${templ_sym_h} ${out_sym_h}
cp ${templ_tok_h} ${out_tok_h}

grep -nP '(?<!define) RET_TOK' ${out_l} | \
	perl -ne \
	'/(^\d+):(\S+)\s+{\s*RET_TOK\s*\(([ \w]+),([ \w]+)/ && print("$1 $2 $3 $4\n")' \
	> ${tmpfile}

function insert_into_header()
{
	prefix="$1"
	item="${1}${2}"
	output="${3}"
	# insert this only if it has not been inserted yet.
	grep "\<${item}\>" ${output} 1>/dev/null || \
		sed -i "/INSERT_HERE/a\\\t${item}," ${output}
}

while read -r line
do
	number=`echo $line | cut -d' ' -f1`
	match=`echo $line | cut -d' ' -f2 | sed -e 's/\\\\//g'` # strip backslashes
	symbol=`echo $line | cut -d' ' -f3`
	token=`echo $line | cut -d' ' -f4`

	# for auto-gen symbol, replace $auto_macro with $match.
	if [ "$symbol" == "$auto_macro" ]; then
		echo -n "S_${match} "
		sed -i "${number}s/${auto_macro}/${match}/" ${out_l}
		insert_into_header "S_" "${match}" "${out_sym_h}"
	else
		echo -n "S_${symbol} "
		insert_into_header "S_" "${symbol}" "${out_sym_h}"
	fi

	# for auto-gen token
	echo -n "T_${token} "
	insert_into_header "T_" "${token}" "${out_tok_h}"
done < ${tmpfile}
echo "done."
