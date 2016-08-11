#!/bin/sh
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Query searchd using curl, for test purpose.
Examples:
$0 ./query-valid.json
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

file="$1"
curl -v -H "Content-Type: application/json" -d @"${file}" "http://localhost:8921/search"
