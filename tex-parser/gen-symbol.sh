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
auto_macro=_AUTOGEN_

cp $lexer_file ${output_lexer}
cp $sym_header ${output_header}

grep -nP '(?<!define) RET_OPTR' ${lexer_file} | \
	awk 'BEGIN {FS=":|,|\\(| +"} {print $1, $2, $5 }' > ${tmpfile}

function insert_into_header()
{
	item="S_${1}"
	output="$2"
	# if this symbol has not been inserted, insert it.
	grep "\<${item}\>" ${output} 1>/dev/null || \
		sed -i "/INSERT_HERE/a\\\t${item}," ${output}
}

while read -r line
do
	number=`echo $line | cut -d' ' -f1`
	match=`echo $line | cut -d' ' -f2 | sed -e 's/\\\\//g'` # strip backslashes
	symbol=`echo $line | cut -d' ' -f3`

	echo "gen token item ${match}..."

	# for auto-gen symbol, replace $auto_macro with $match.
	if [ "$symbol" == "$auto_macro" ]; then
		sed -i "${number}s/${auto_macro}/${match}/" ${output_lexer}
		insert_into_header "${match}" "${output_header}"
	else
		insert_into_header "${symbol}" "${output_header}"
	fi
done < ${tmpfile}
