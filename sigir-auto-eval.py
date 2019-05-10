#!/usr/bin/python3
import subprocess
import _thread
import signal
import time
import sys
import csv
import os

# setup python script signal handler
def signal_handler(sig, _):
    global daemon
    daemon.send_signal(signal.SIGINT)
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

index_dir = "/home/tk/nvme0n1/mnt-%s.img"
cache_lst = "~/nvme0n1/cache-list.tmp"

def ensure_no_daemon():
    pid_out = os.popen('pidof searchd.out').read()
    pid_str = pid_out.strip()
    if len(pid_str):
        os.system('kill -INT ' + pid_str)
        time.sleep(3)

def feed_cachelist(medium):
    os.system('rm -f ./searchd/cache-list.tmp')
    if 'memo' in  medium:
        os.system('cp %s ./searchd/cache-list.tmp' % cache_lst)

checking = False
def check_alive(daemon):
    global checking
    while checking:
        line_bytes = daemon.stdout.readline() # blocking
        line = str(line_bytes, 'utf-8')
        print(line.rstrip())

def do_evaluation(run_file, medium, index):
    # Run make
    res = subprocess.run(['make'])
    if 0 != res.returncode:
        sys.exit(0)

    # setup parameters
    ensure_no_daemon()
    feed_cachelist(medium)

    # Run searchd
    daemon = subprocess.Popen(
        ['./run/searchd.out',
         "-i", index_dir % index,
         '-c', str(0),
         "-T"
        ], stdout=subprocess.PIPE,
           cwd='./searchd')

    while True:
        line_bytes = daemon.stdout.readline() # blocking
        line = str(line_bytes, 'utf-8')
        print(line)
        if "listening" in line:
            break

    global checking
    checking = True
    _thread.start_new_thread(check_alive, (daemon, ))

    # Run queries
    os.system('rm -f %s ' % './searchd/merge.runtime.dat') #########

    time_file = run_file + '.runtime.dat'
    trec_file = run_file + '.trec.dat'
    time_fh = sys.stdout
    trec_fh = open(trec_file, 'w')
    print('--- BEGIN --- ')
    res = subprocess.run(['./genn-trec-results.py'], stderr=time_fh, stdout=trec_fh)
    print('--- FINISH --- ')
    #time_fh.close()
    trec_fh.close()

    os.system('mv %s %s' % ('./searchd/merge.runtime.dat', time_file)) #########

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
    checking = False
    time.sleep(3)
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
input_tsv = "eff.tsv"
with open(input_tsv) as fd:
    tot_rows = sum(1 for _ in open(input_tsv))
    rd = csv.reader(fd, delimiter="\t")
    replaces = dict()
    for idx, row in enumerate(rd):
        if idx == 0: continue # skip the header row
        run_name = row[0]
        print('[row %u / %u] %s' % (idx + 1, tot_rows, run_name))
        if os.path.exists('./tmp/' + run_name + '.eval'):
            print('skip this row')
            time.sleep(0.1)
            continue
        print(row)
        time.sleep(1)
        replaces["top"] = row[1]
        replaces["threshold"] = float_str(row[2])
        replaces["strategy"] = row[3].upper()
        medium = row[4]
        index = row[5]
        replace_source_code(replaces)
        do_evaluation('./tmp/' + run_name, medium, index)
        break;
