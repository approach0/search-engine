#!/usr/bin/python3
import json
import requests
import time
import sys

url= 'http://localhost:8934/index'
#url= 'http://localhost:8998/parser'

fname = "ntcir12.tmp"
#fname = "small.tmp"

def file_len(fname):
	with open(fname) as f:
		for i, l in enumerate(f):
			pass
	return i + 1
total_lines = file_len(fname)

def send_json(json_obj):
	headers = {'content-type': 'application/json'}
	r = requests.post(url, json=json_obj, headers=headers)
	j = json.loads(r.content.decode("utf-8"))
	return j['docid']

with open(fname) as f:
	cnt = 0
	for line in f:
		line = line.strip('\n')
		fields = line.split()
		docid_and_pos = fields[0]
		latex = ' '.join(fields[1:])
		latex = latex.replace('% ', '')
		docid = docid_and_pos

		json_obj = {}
		json_obj['url'] = docid;
		json_obj['text'] = '[imath]' + latex + '[/imath]  ';
		json_obj['tex'] = latex;
		# print(latex)
		a0_docid = send_json(json_obj)
		print("%d  %s  tex: %s" % (a0_docid, docid, latex), file=sys.stderr)

		cnt += 1
		print("%d / %d" % (cnt, total_lines))
