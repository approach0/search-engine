#!/bin/bash
if [ $# -eq 1 ] && [ $1 == '-h' ]
then
	echo 'DESCRIPTION:'
	echo 'auto-generate token/symbol translation .c file.'
	echo ''
	echo 'SYNOPSIS:'
	echo "$0 <symbol header> <token header> <C template file> <output file>"
	echo ''
	echo 'OUTPUT:'
	echo "<output file>"
	exit
fi

[ $# -ne 4 ] && echo "bad arg." && exit
[ ! -e ${1} ] && echo "$1 not exists" && exit
[ ! -e ${2} ] && echo "$2 not exists" && exit
[ ! -e ${3} ] && echo "$3 not exists" && exit

sym_header="${1}"
tok_header="${2}"
C_template="${3}"
output="${4}"
tmpfile='trans-gen.tmp'

cp $C_template ${output}

gen_fun() {
	prefix=${1}
	header=${2}
	grep -oP '(?<='"${prefix}"')[\w ]+(?=,|=)' ${header} > ${tmpfile}

	while read name; do 
		echo -n "${prefix}${name} "
		sed -i "/${prefix}INSERT_HERE/a \
		\\\tcase ${prefix}${name}:\n \
		\tsprintf(ret, \"${name}\");\n \
		\tbreak;\n" \
			   ${output}
	done < ${tmpfile}
}

gen_fun S_ ${sym_header}
gen_fun T_ ${tok_header}
echo "done."
