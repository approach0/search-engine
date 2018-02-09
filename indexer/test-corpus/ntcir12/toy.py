#!/usr/bin/python

with open("toy.txt") as f:
    for line in f:
        line = line.strip('\n')
        fields = line.split()
        docid = fields[0]
        latex = ' '.join(fields[1:])
        print(docid)
        print(latex)
