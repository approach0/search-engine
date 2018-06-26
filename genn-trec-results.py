#!/usr/bin/python3
import os
import time
import sys

# ./search/run/test-search.out -n -i ./tmp/ -m 'a+b' -q 'NTCIR12-MathWiki-123'

fname = "indexer/test-corpus/ntcir12/topics-concrete.txt"
index = "/home/tk/Desktop/approach0/indexer/tmp"
output = 'trec-format-results.tmp'

rmcmf = 'rm -f {}'.format(output)
os.system(rmcmf);

with open(fname) as f:
    for line in f:
        print(line)
        line = line.strip('\n')
        fields = line.split()
        qry_id = fields[0]
        latex = ' '.join(fields[1:])
        #exestr = "./search/run/test-search.out -p 0 -n -i {} -m '{}' -q {}".format(index, latex, qry_id)
        exestr = "./searchd/scripts/mathonly-qry.py '{}' {}".format(latex, qry_id)
        print(exestr)
        t0 = time.time()
        os.system(exestr);
        t1 = time.time()
        sys.stderr.write('%s %f seconds \n' % (qry_id, t1 - t0))
        sys.stderr.flush()

print('output file: %s' % output)
