#!/usr/bin/python
import json
import requests
import time

fname = "NTCIR12_latex_expressions.txt"
#fname = "toy.txt"

def file_len(fname):
	with open(fname) as f:
		for i, l in enumerate(f):
			pass
	return i + 1
total_lines = file_len(fname)

def send_json(json_obj):
	headers = {'content-type': 'application/json'}
	r = requests.post('http://localhost:8934/index',
		json=json_obj,
		headers=headers)
	# print(r.status_code)

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
		# print(latex)
		send_json(json_obj)

		cnt += 1
		print("%d / %d" % (cnt, total_lines))
