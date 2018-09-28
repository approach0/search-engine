#!/usr/bin/python3
import subprocess
import signal
import time
import sys

# setup python script signal handler
def signal_handler(sig, _):
    global daemon
    daemon.send_signal(signal.SIGINT)
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

proj_dir  = "/home/tk/Desktop/approach0"
index_dir = "/home/tk/index-fix-decimal-and-use-latexml/"

def do_evaluation(run_file):
    # Run make
    res = subprocess.run(['make'])
    if 0 != res.returncode:
        sys.exit(0)

    # Run searchd
    daemon = subprocess.Popen(
        ['./run/searchd.out',
         "-i", index_dir,
         '-c', str(32),
         '-T'
        ], stdout=subprocess.PIPE,
           cwd=proj_dir + '/searchd')

    while True:
        line_bytes = daemon.stdout.readline()
        line = str(line_bytes, 'utf-8')
        print(line)
        if "listening" in line:
            break

    # Run queries
    trec_file = run_file + '.dat'
    fh = open(trec_file, 'w')
    res = subprocess.run(['./genn-trec-results.py'], stdout=fh)
    fh.close()

    # Run evaluations
    fh = open(run_file + '.eval', 'w')
    res = subprocess.run(
        ['./eval-trec-results-summary.sh', trec_file], stdout=fh)
    fh.close()

    # close daemon
    daemon.send_signal(signal.SIGINT)

templates = [
    {
        "path": "./tmp/template/config.h",
        "output": "./search/config.h"
    },
    {
        "path": "./tmp/template/math-expr-sim.c",
        "output": "./search/math-expr-sim.c"
    },
]

def replace_source_code(replaces):
    global templates
    for t in templates:
        fh_t = open(t["path"], 'r')
        t['txt'] = fh_t.read()
        fh_t.close()
    for k, v in replaces.items():
        print(k, v)
        for t in templates:
            t['txt'] = t['txt'].replace('{{' + k + '}}', v)
    for t in templates:
        with open(t['output'], 'w') as f:
            f.write(t['txt'])

import math
def float_str(string):
    v = float(string)
    if math.floor(v) == v:
        return string + '.f'
    else:
        return string + 'f'

##
## Main procedure
##
import csv
import os

input_tsv = "test.tsv"
with open(input_tsv) as fd:
    tot_rows = sum(1 for _ in open(input_tsv))
    rd = csv.reader(fd, delimiter="\t")
    replaces = dict()
    for idx, row in enumerate(rd):
        run_name = row[0]
        print('row %u / %u ...' % (idx + 1, tot_rows))
        if os.path.exists('./tmp/' + run_name + '.eval'):
            print('skip this row')
            time.sleep(0.5)
            continue
        replaces["theta"]  = float_str(row[1])
        replaces["mc"]     = row[2]
        replaces["alpha"]  = float_str(row[3])
        replaces["beta_1"] = float_str(row[4])
        replaces["beta_2"] = float_str(row[5])
        replaces["beta_3"] = float_str(row[6])
        replaces["beta_4"] = float_str(row[7])
        replaces["beta_5"] = float_str(row[8])
        k = 1;
        for i in range(1, 6):
            if float(row[i + 3]) == 0:
                k = i - 1
                break
        replaces["K"] = str(k)
        replace_source_code(replaces)
        do_evaluation('./tmp/' + run_name)
