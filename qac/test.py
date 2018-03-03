#!/usr/bin/python
import json
import requests
import time

fname = "test.tmp"
urlbase = 'http://localhost:8913'

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

total_lines = file_len(fname)
g_cnt = 0

def send_json(uri, json_obj):
    global g_cnt
    headers = {'content-type': 'application/json'}
    r = requests.post(urlbase + uri,
        json=json_obj,
        headers=headers)
    g_cnt += 1
    print(r.status_code, end=": ")
    print("%d / %d" % (g_cnt, total_lines))

with open(fname) as f:
    for line in f:
        latex = line.rstrip()
        print(latex);
        json_obj = {"tex": latex}
        print(json_obj);
        send_json('/post_log', json_obj)
