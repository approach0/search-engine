#!/usr/bin/python3
import csv
import os
import json

input_tsv = "eff.tsv"

with open(input_tsv) as fd:
    rd = csv.reader(fd, delimiter="\t")
    for idx, row in enumerate(rd):
        run_name = row[0]
        in_file = './tmp/' + run_name + '.stats.dat'
        if not os.path.exists(in_file):
            # print("%s does not exist" % in_file)
            continue
        in_fh = open(in_file)
        content = in_fh.read()
        j = json.loads(content)
        res = [("%.2f" % j[f]) for f in ['mean', 'median', 'max', 'min', 'variance']]
        res += [("%.3f" % t) for t in j['runtime']]
        head=row[0:4]
        print('\t'.join(head), '\t', '\t'.join(res))
        in_fh.close()
