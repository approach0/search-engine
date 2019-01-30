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

index_dir = "/home/tk/nvme0n1/mnt-math-symb-unicode-range-fix.img/"

def do_evaluation(run_file):
    # Run make
    res = subprocess.run(['make'])
    if 0 != res.returncode:
        sys.exit(0)

    # Run searchd
    daemon = subprocess.Popen(
        ['./run/searchd.out',
         "-i", index_dir,
         '-c', str(0),
         "-T" # Delete this option if you do not want to include writing overhead.
        ], stdout=subprocess.PIPE,
           cwd='./searchd')

    while True:
        line_bytes = daemon.stdout.readline()
        line = str(line_bytes, 'utf-8')
        print(line)
        if "listening" in line:
            break

    # Run queries
    time_file = run_file + '.runtime.dat'
    trec_file = run_file + '.trec.dat'
    time_fh = open(time_file, 'w')
    trec_fh = open(trec_file, 'w')
    res = subprocess.run(['./genn-trec-results.py'], stderr=time_fh, stdout=trec_fh)
    time_fh.close()
    trec_fh.close()

    # Run evaluations
    fh = open(run_file + '.eval.dat', 'w')
    res = subprocess.run(
        ['./eval-trec-results-summary.sh', trec_file], stdout=fh)
    fh.close()

    # Run stats
    fh = open(run_file + '.stats.dat', 'w')
    res = subprocess.run(
        ['python3', './stats-efficiency-summary.py', time_file], stdout=fh)
    fh.close()

    # close daemon
    daemon.send_signal(signal.SIGINT)

templates = [
    {
        "path": "./tmp/template/config.h",
        "output": "./search/config.h"
    }
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

input_tsv = "eff.tsv"
with open(input_tsv) as fd:
    tot_rows = sum(1 for _ in open(input_tsv))
    rd = csv.reader(fd, delimiter="\t")
    replaces = dict()
    for idx, row in enumerate(rd):
        if idx == 0: continue # skip the header row
        # if idx == 2: break # for debug
        run_name = row[0]
        print('[row %u / %u] %s' % (idx + 1, tot_rows, run_name))
        if os.path.exists('./tmp/' + run_name + '.eval'):
            print('skip this row')
            time.sleep(0.1)
            continue
        replaces["symbol"] = row[1].upper()
        replaces["prune"]  = row[2].upper()
        replaces["skip"]   = row[3].upper()
        replaces["factor"] = float_str(row[4])
        replace_source_code(replaces)
        do_evaluation('./tmp/' + run_name)
        # break
