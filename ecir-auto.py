#!/usr/bin/python3
import subprocess
import signal
import time
import sys

proj_dir  = "/home/tk/Desktop/approach0"
index_dir = "/home/tk/index-fix-decimal-and-use-latexml/"

# setup python script signal handler
def signal_handler(sig, _):
    global daemon
    daemon.send_signal(signal.SIGINT)
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def evaluation(run_file)
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
