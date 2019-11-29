#!/usr/bin/python3
import json
import argparse
import requests
import time
import sys
import os

def send_json(json_obj, url):
	headers = {'content-type': 'application/json'}
	r = requests.post(url, json=json_obj, headers=headers)
	j = json.loads(r.content.decode("utf-8"))
	return j['docid']

def each_json_file(corpus, endat):
	cnt = 0
	for dirname, dirs, files in os.walk(corpus):
		for f in files:
			if cnt >= endat and endat > 0:
				return
			if f.split('.')[-1] == 'json':
				yield (dirname, f)
			cnt += 1

def get_n_files(corpus):
	cnt = 0
	for dirname, basename in each_json_file(corpus, -1):
		path = dirname + '/' + basename
		cnt += 1
	return cnt

# main #
default_url = 'http://localhost:8934/index'
parser = argparse.ArgumentParser(description='Approach0 indexd json feeder.')
parser.add_argument('--begin-from', help='begin from specified count of files.', type=int)
parser.add_argument('--end-at', help='stop at specified count of files.', type=int)
parser.add_argument('--corpus-path', help='corpus path. (required)', type=str)
parser.add_argument('--indexd-url', help='indexd URL. Default: ' + default_url, type=str)
args = parser.parse_args()

url= args.indexd_url if args.indexd_url else default_url
corpus = args.corpus_path if args.corpus_path else ''
endat = args.end_at if args.end_at else -1
begin = args.begin_from if args.begin_from else 0

print('Indexd URL: ' + url)

print('Count how many files there ...')
N = get_n_files(corpus)
print('%u files in total.' % N)

cnt = 0
for dirname, basename in each_json_file(corpus, endat):
	cnt += 1
	if cnt < begin:
		continue
	path = dirname + '/' + basename
	with open(path, 'r') as fh:
		try:
			j = json.load(fh)
			docid = send_json(j, url)
		except Exception as err:
			print(err)
			break
		print(f'[{cnt:,d} / {N:,d}] doc#{docid}: {j["url"]}')
