#!/bin/sh
file=$1
curl -v -H "Content-Type: application/json" -d @"${file}" "http://localhost:3207/push/query"

curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/30"
