#!/usr/bin/python3
import json
import argparse
import requests
import time
import sys
import os

def send_json(json_obj):
	headers = {'content-type': 'application/json'}
	r = requests.post(url, json=json_obj, headers=headers)
	j = json.loads(r.content.decode("utf-8"))
	return j['docid']

def each_json_file(corpus, maxcnt):
	cnt = 0
	for dirname, dirs, files in os.walk(corpus):
		for f in files:
			if cnt >= maxcnt and maxcnt > 0:
				return
			yield (dirname, f)
			cnt += 1

def get_n_files(corpus):
	cnt = 0
	for dirname, basename in each_json_file(corpus, -1):
		path = dirname + '/' + basename
		cnt += 1
	return cnt

# main #
parser = argparse.ArgumentParser(description='Approach0 indexd json feeder.')
parser.add_argument('--begin-from', help='begin from specified count of files.', type=int)
parser.add_argument('--maxfiles', help='limit the max number of files to be indexed.', type=int)
parser.add_argument('--corpus-path', help='corpus path. (required)', type=str)
parser.add_argument('--indexd-url', help='indexd URL.', type=str)
args = parser.parse_args()

url= args.indexd_url if args.indexd_url else 'http://localhost:8934/index'
corpus = args.corpus_path if args.corpus_path else ''
maxfiles = args.maxfiles if args.maxfiles else -1
begin = args.begin_from if args.begin_from else 0

print('Indexd URL: ' + url)

print('Count how many files there ...')
N = get_n_files(corpus)
print('%u files in total.' % N)

cnt = 0
for dirname, basename in each_json_file(corpus, maxfiles):
	cnt += 1
	if cnt < begin:
		continue
	path = dirname + '/' + basename
	with open(path, 'r') as fh:
		try:
			j = json.load(fh)
			docid = send_json(j)
		except Exception as err:
			print(err)
			break
		print(f'[{cnt:,d} / {N:,d}] doc#{docid}: {j["url"]}')
