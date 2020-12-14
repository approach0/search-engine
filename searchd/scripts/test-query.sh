#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Query searchd using curl, for test purpose.
Examples:
$0 http://localhost:8921/search ./tests/query-valid.json
USAGE
exit
fi

script_path=$(cd `dirname $0` && pwd)

url="${1-http://localhost:8921/search}"
file="${2-$script_path/../tests/query-valid.json}"

echo "[API URL] $url"
cat $file

curl -v -H "Content-Type: application/json" -d @"${file}" "${url}"
echo ""
