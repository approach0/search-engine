#!/usr/bin/python3
import os
import time
import sys

# ./search/run/test-search.out -n -i ./tmp/ -m 'a+b' -q 'NTCIR12-MathWiki-123'

fname = "indexer/test-corpus/ntcir12/topics-concrete.txt"
#index = "/home/tk/Desktop/approach0/indexer/tmp"
output = 'searchd/trec-format-results.tmp'

only_qry_id = None
# only_qry_id = 'NTCIR12-MathWiki-4'

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
        os.system(exestr);
        t1 = time.time()
        # report query process time
        sys.stderr.write('%s %f seconds \n' % (qry_id, t1 - t0))
        sys.stderr.flush()
        # print TREC output
        try:
            with open(output) as ff:
                trec_output = ff.read()
                trec_output = trec_output.replace("_QRY_ID_", qry_id)
                print(trec_output, end='')
        except:
            pass
