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
cnt = 0

with open(fname) as f:
    for line in f:
        line = line.strip('\n')
        fields = line.split()
        docid = fields[0]
        latex = ' '.join(fields[1:])
        json_obj = {
            "text": '[imath]' + latex + '[/imath]',
            "url": docid
        }
        print(json_obj)

        headers = {'content-type': 'application/json'}
        r = requests.post('http://localhost:8934/index',
            json=json_obj,
            headers=headers)
        print(r.status_code)
        # time.sleep(.10)
        print("%d / %d" % (cnt, total_lines))
        cnt += 1

