#!/bin/sh
curl -v -H "Content-Type: application/json" -d daniel "http://localhost:8914/hello"
curl -v -H "Content-Type: application/json" -d lousy  "http://localhost:8914/world"
