#!/bin/sh
file=$1
range="0000-01-01.9999-12-31"
curl -v -H "Content-Type: application/json" -d @"${file}" "http://localhost:3207/push/query"
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-IPs/30"
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-summary/c.b"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/30/$range"
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/from-127.0.0.1/30/$range"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/ip-info/121.32.198.208"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/push/ipinfo/20.32.198.208"
