#!/bin/sh
range="0000-01-01.9999-12-31"

## Insertions
# curl -v -H "Content-Type: application/json" -d @"test-1.json" "http://localhost:3207/push/query"
# curl -v -H "Content-Type: application/json" -d @"test-2.json" "http://localhost:3207/push/query"
# curl -v -H "Content-Type: application/json" -d @"test-3.json" "http://localhost:3207/push/query"

## Query summary
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-summary/$range"

## Query IPs
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-IPs/30/$range"
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-IPs/from-121.32.198.208/30/$range"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/30/$range"
# curl -v -H "Content-Type: application/json" "http://localhost:3207/pull/query-items/from-121.32.198.208/30/$range"

# curl -v -H "Content-Type: application/json" "http://localhost:3207/push/ipinfo/20.32.198.208"
