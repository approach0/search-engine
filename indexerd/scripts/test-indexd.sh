#!/bin/sh
curl -vX POST http://localhost:8934/index -d @"test.json" \
	--header "Content-Type: application/json"
