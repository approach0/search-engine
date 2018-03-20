#!/usr/bin/python
import json
import requests
import time
import sys

fname = "test.tmp"
urlbase = 'http://localhost:8913'

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

def send_json(uri, json_obj):
    headers = {'content-type': 'application/json'}
    r = requests.post(urlbase + uri,
        json=json_obj,
        headers=headers)
    print(r.status_code, end=": ")

def post_query_logs(limit):
    total_lines = file_len(fname)
    cnt = 0
    with open(fname) as f:
        for line in f:
            latex = line.rstrip()
            print(latex);
            json_obj = {"tex": latex}
            print(json_obj);
            send_json('/post_log', json_obj)
            cnt += 1
            print("%d / %d" % (cnt, total_lines))
            if cnt >= limit:
                break

def test_qac_query(qry_tex):
    json_obj = {"tex": qry_tex}
    print(json_obj);
    send_json('/qac_query', json_obj)

# post_query_logs(10000)
# post_query_logs(sys.maxsize)

test_qac_query('a^2')
#test_qac_query('1+1/n')
