#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Query searchd using curl, for test purpose.
Examples:
$0 "http://localhost:8921/search" ./tests/query-valid.json 
USAGE
exit
fi

[ $# -ne 2 ] && echo 'bad arg.' && exit

url="$1"
file="$2"
cat $file
curl -v -H "Content-Type: application/json" -d @"${file}" "${url}"
