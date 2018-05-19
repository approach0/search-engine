#!/usr/bin/python3
import json
import requests
import time
import sys
import os

run = 'full-professor'
output = 'trec-format-results.tmp'

def get_latex_list(queryID):
	headers = {'content-type': 'plain/text'}
	url = "http://localhost:3838/get/%s/%s/latex.list" % (run, queryID)
	print('[curl]', url)
	r = requests.get(url=url, headers=headers)
	print(r.status_code)
	if r.status_code == 200:
		text = r.content.decode("utf-8")
	else:
		text = ''
	return text

def create_trec_corpus():
	os.system('rm -rf ./trec-index.tmp')
	for qnum in range(1, 11):
		queryID = 'NTCIR12-MathWiki-' + str(qnum)
		text = get_latex_list(queryID)
		for idx, line in enumerate(text.split('\n')):
			line = line.strip('\n')
			fields = line.split()
			if len(fields) == 0:
				break
			docid = fields[0]
			latex = ' '.join(fields[1:])
			# Now create JSON
			json_obj = {}
			json_obj['url'] = docid;
			json_obj['text'] = '[imath]' + latex + '[/imath]  ';
			# Create Path
			path = './trec-index.tmp/' + queryID
			os.system('mkdir -p ' + path)
			# Create File
			fh = open(path + '/' + str(idx) + '.json', 'w')
			json.dump(json_obj, fh)
			fh.close()

def get_query(queryID):
	headers = {'content-type': 'plain/text'}
	url = "http://localhost:3838/get/%s/query.tex" % queryID
	print('[curl]', url)
	r = requests.get(url=url, headers=headers)
	print(r.status_code)
	if r.status_code == 200:
		j = json.loads(r.content.decode("utf-8"))
		query = j['qrytex'];
	else:
		query = ''
	return query

def create_trec_results():
	os.system('ln -sf ../../run/indexer.out')
	os.system('ln -sf ../../../search/run/test-search.out')
	os.system('rm -f %s' % output)
	for qnum in range(1, 11):
		queryID = 'NTCIR12-MathWiki-' + str(qnum)
		query = get_query(queryID)
		if query == '':
			continue
		print('[searching]', queryID)
		foo = os.system # print
		foo('./indexer.out -p ./trec-index.tmp/%s' % queryID)
		foo('./test-search.out -p 0 -n -i ./tmp/ -m "%s" -q %s > /dev/null' % (query, queryID))
	print('output:', output)

create_trec_corpus()
create_trec_results()
