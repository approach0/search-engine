#!/usr/bin/python3
import requests
import json
import sys

url = 'http://localhost:8921/search'

query = sys.argv[1] #'\\frac a b'
qryID = sys.argv[2]

def send_json(obj):
	headers = {'content-type': 'application/json'}
	r = requests.post(url, json=obj, headers=headers)
	j = json.loads(r.content.decode('utf-8'))
	print(j)


json_query = {
	"page": 1,
	"id": qryID,
	"kw": [
		{"type": "tex", "str": query}
	]
}

send_json(json_query)
