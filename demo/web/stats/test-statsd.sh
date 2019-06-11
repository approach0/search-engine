#!/bin/sh
file=$1
curl -v -H "Content-Type: application/json" -d @"${file}" "http://localhost:3207/push/query"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/30"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/30/2018-09-09.2019-09-09"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-IPs/30"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-summary"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-summary/c.b"
