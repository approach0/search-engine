#!/usr/bin/python3
import csv
import os
import json

input_tsv = "eff.tsv"

def parse_evaluation_results(lines):
    res_arr = []
    partial_rel = False
    for l in lines:
        fields = l.split()
        if len(fields) == 3:
            value = fields[2]
            res_arr.append(value)
        elif fields[0] == "Partial":
            partial_rel = True
        elif fields[0] == "Full":
            partial_rel = False
        else:
            print('unexpected fields')
            print(fields)
    return res_arr

with open(input_tsv) as fd:
    rd = csv.reader(fd, delimiter="\t")
    for idx, row in enumerate(rd):
        run_name = row[0]
        res = []
        # Read eval file
        eval_file = './tmp/' + run_name + '.eval.dat'
        if not os.path.exists(eval_file):
            res += ["-1.f", "-1.f"]
        else:
            with open(eval_file) as in_fh:
                a = parse_evaluation_results(in_fh.readlines())
                res.append(a[0])
                res.append(a[0 + 5])
        # Read stats file
        stats_file = './tmp/' + run_name + '.stats.dat'
        if not os.path.exists(stats_file):
            # print("%s does not exist" % stats_file)
            continue
        with open(stats_file) as in_fh:
            content = in_fh.read()
            j = json.loads(content)
            res += [("%.2f" % j[f]) for f in ['mean', 'median', 'max', 'min', 'variance']]
            res += [("%.3f" % t) for t in j['runtime']]
        head=row[0:4]
        print('\t'.join(head), '\t', '\t'.join(res))
