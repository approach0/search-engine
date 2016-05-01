#!/bin/bash
if [ $# -eq 1 ] && [ $1 == '-h' ]
then
	echo 'DESCRIPTION:'
	echo 'auto-generate auto-completion implementation .c file.'
	echo ''
	echo 'SYNOPSIS:'
	echo "$0 <.l> <template .c> <output .c>"
	echo ''
	echo 'OUTPUT:'
	echo "<output .c>"
	exit
fi

[ $# -ne 3 ] && echo "bad arg." && exit
[ ! -e ${1} ] && echo "$1 not exists" && exit
[ ! -e ${2} ] && echo "$2 not exists" && exit

lex_l="${1}"
temp_c="${2}"
out_c="${3}"

tmpfile=completion-gen.tmp

cp ${temp_c} ${out_c}

cat ${lex_l} | grep -Po \
	'^\\\\[a-zA-Z]+' \
	> ${tmpfile}

sort ${tmpfile} | uniq > ${tmpfile}.tmp
mv -f ${tmpfile}.tmp ${tmpfile}

while read -r term
do
	echo -n "$term "
	sed -i "/INSERT_HERE/a\\\t\"\\\\${term}\"," ${out_c}
done < ${tmpfile}

echo "done."
