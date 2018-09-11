#!/bin/sh
cat ${1:-/dev/stdin} | grep 'NTCIR12-' | awk -v ORS="," '{print $3}'
echo ""
