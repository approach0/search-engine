#!/usr/bin/python3
import os

# ./search/run/test-search.out -n -i ./tmp/ -m 'a+b' -q 'NTCIR12-MathWiki-123'

#fname = "indexer/test-corpus/ntcir12/topics.txt"
fname = "indexer/test-corpus/ntcir12/topics-concrete.txt"
index = "/home/tk/Desktop/approach0/indexer/tmp"
#index = "/home/tk/rotate-disk/ext4/index-prefix-ntcir12-fix-bottleneck"
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
        exestr = "./search/run/test-search.out -p 0 -n -i {} -m '{}' -q {}".format(index, latex, qry_id)
        print(exestr)
        os.system(exestr);

print('output file: %s' % output)
