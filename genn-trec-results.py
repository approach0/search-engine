#!/usr/bin/python3
import os
# ./search/run/test-search.out -n -i ./tmp/ -m 'a+b' -q 'NTCIR12-MathWiki-123'

fname = "indexer/test-corpus/ntcir12/topics.txt"

os.system('rm -f trec_fmt_result.txt')

with open(fname) as f:
    for line in f:
        print(line)
        line = line.strip('\n')
        fields = line.split()
        qry_id = fields[0]
        latex = ' '.join(fields[1:])
        exestr = "./search/run/test-search.out -n -i ./tmp/ -m '{}' -q {}".format(latex, qry_id)
        print(exestr)
        os.system(exestr);
