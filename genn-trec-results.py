#!/usr/bin/python3
import os
import time
import sys

# ./search/run/test-search.out -n -i ./tmp/ -m 'a+b' -q 'NTCIR12-MathWiki-123'

fname = "/home/tk/rotate-disk/ext4/ntcir-12/topics-concrete.txt"
output = 'searchd/trec-format-results.tmp'

only_qry_id = None

if len(sys.argv) == 2:
    only_qry_id = 'NTCIR12-MathWiki-' + sys.argv[1]

with open(fname) as f:
    for line in f:
        line = line.strip('\n')
        # parse and prepare command arguments
        fields = line.split()
        qry_id = fields[0]
        if only_qry_id and only_qry_id != qry_id:
            continue
        latex = ' '.join(fields[1:])
        exestr = "./searchd/scripts/mathonly-qry.py '{}' {}".format(latex, qry_id)
        if len(sys.argv) > 1 and sys.argv[1] == '--dry-run':
            print(exestr)
            continue
        # execute
        t0 = time.time()
        ret = os.system(exestr);
        t1 = time.time()
        # report query process time
        sys.stderr.write('%s %f msec \n' % (qry_id, (t1 - t0) * 1000.0))
        sys.stderr.flush()
        if ret != 0: sys.exit(1)
        # print TREC output
        try:
            with open(output) as ff:
                trec_output = ff.read()
                trec_output = trec_output.replace("_QRY_ID_", qry_id)
                print(trec_output, end='')
        except:
            pass
