#!/usr/bin/python3
import os

fname = "trec-format-results.tmp"
docdict = dict()
doclist = list()

with open(fname) as f:
    for line in f:
        fields = line.split();
        docid = fields[2]
        score = fields[4]
        if docid in docdict:
            continue
        #fields[2] += ":2"
        #fields[4] = "0.9"
        doclist.append(fields)
        docdict[docid] = 1

for item in doclist:
    string = ' '.join(item)
    print(string)
