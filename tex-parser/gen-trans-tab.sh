#!/bin/bash
if [ $# -eq 1 ] && [ $1 == '-h' ]
then
	echo 'DESCRIPTION:'
	echo 'auto-generate token/symbol translation .c file.'
	echo ''
	echo 'SYNOPSIS:'
	echo "$0 <symbol header> <token header> <.c file>"
	echo ''
	echo 'OUTPUT:'
	echo "gen-<.c file>"
	exit
fi

[ $# -ne 3 ] && echo "bad arg." && exit
[ ! -e ${1} ] && echo "$1 not exists" && exit
[ ! -e ${2} ] && echo "$2 not exists" && exit
[ ! -e ${3} ] && echo "$3 not exists" && exit

sym_header="${1}"
tok_header="${2}"
c_original="${3}"
tmpfile='name-list.tmp'

cp $c_original gen-${c_original}

gen_fun() {
	prefix=${1}
	header=${2}
	grep -oP '(?<='"${prefix}"').*(?=,)' ${header} > ${tmpfile}

	while read name 
	do 
		echo "${prefix}: gen translation item ${name}"
		sed -i "/${prefix}INSERT_HERE/a \
		\\\tcase ${prefix}${name}:\n \
		\tsprintf(ret, \"${name}\");\n \
		\tbreak;\n" \
			   gen-${c_original}
	done < ${tmpfile}
}

gen_fun S_ ${sym_header}
gen_fun T_ ${tok_header}
