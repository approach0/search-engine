#!/usr/bin/python3
import os

fname = "trec-format-results.tmp"
run_name = 'a0-ntcir-tmp'

d = dict()
out_d = dict()

with open(fname) as f:
    for line in f:
        fields = line.split();
        query = fields[0]
        docid = fields[2]
        rank  = fields[3]
        score = fields[4]
        run = run_name
        key = query + docid;
        if key in d:
            continue
        else:
            d[key] = True
        if query in out_d:
            l = out_d[query]
            rank = len(l) + 1
        else:
            out_d[query] = list()
            rank = 1
        hit = [query, "1", docid, str(rank), score, run]
        out_d[query].append(hit)

order_key = list(out_d.keys())
order_key = sorted(order_key, key=lambda x: int(x.split('-MathWiki-')[1]))
for k in order_key:
    l = out_d[k]
    for res in l:
        line = ' '.join(res)
        print(line)
