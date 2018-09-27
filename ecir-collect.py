#!/usr/bin/python3
import csv
import os

input_tsv = "test.tsv"

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
        run_eval = './tmp/' + run_name + '.eval'
        if not os.path.exists(run_eval):
            print("%s does not exist" % run_eval)
            continue
        eval_fh = open(run_eval)
        res = parse_evaluation_results(eval_fh.readlines())
        print('\t'.join(row), '\t', '\t'.join(res))
        eval_fh.close()
