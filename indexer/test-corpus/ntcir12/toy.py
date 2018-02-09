#!/usr/bin/python
import json
import requests
import time

fname = "NTCIR12_latex_expressions.txt"
#fname = "toy.txt"

limit = 100
first_time = True

def file_len(fname):
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1
total_lines = file_len(fname)

g_cnt = 0

def send_json(json_obj):
    global g_cnt
    headers = {'content-type': 'application/json'}
    r = requests.post('http://localhost:8934/index',
        json=json_obj,
        headers=headers)
    print(r.status_code)
    # time.sleep(.10)
    g_cnt += 1
    print("%d / %d" % (g_cnt, total_lines))
    if limit != -1 and g_cnt > limit:
        quit()

with open(fname) as f:
    last_docid = 'undefined'
    json_obj = {"text": '', "url": 'undefined'}

    for line in f:
        line = line.strip('\n')
        fields = line.split()
        docid_and_pos = fields[0]
        latex = ' '.join(fields[1:])
        docid = docid_and_pos.split(':')[0]
        pos = docid_and_pos.split(':')[1]

        if first_time or docid == last_docid:
            first_time = False
        else:
            print(json_obj)
            send_json(json_obj)
            json_obj = {"text": '', "url": 'undefined'}

        json_obj['url'] = docid;
        json_obj['text'] += '[imath]' + latex + '[/imath]  ';
        last_docid = docid
