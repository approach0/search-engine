#!/usr/bin/python3
import sys
import json
import requests

topics = '/home/tk/Desktop/approach-trec/trecfiles/topics.txt'
corpus = '/home/tk/Desktop/approach-trec/trecfiles/corpus.txt'
hi_url = "http://localhost:8961/highlight"
limit_line = -1
suffix_operands = 'hi-operands.dat'
suffix_trees = 'hi-trees.dat'

if len(sys.argv) != 2:
    print('bad args')
    quit()

def send_json(obj, url):
    headers = {'content-type': 'application/json'}
    r = requests.post(url, json=obj, headers=headers)
    j = json.loads(r.content.decode("utf-8"))
    return j

def parse_annotat(field):
    # format: "docid|qmask[3]|dmask[3]"
    # example: "91080|64008,82,104|1e,c0000,300000"
    bar_fields = field.split('|')
    # parse sub-fields
    doc_id = int(bar_fields[0])
    qmasks = bar_fields[1].split(',')
    dmasks = bar_fields[2].split(',')
    return (doc_id, qmasks, dmasks)

def print_graph_highlight(key, qry_tex, doc_id, fh):
    hi_req = {
        "query_type": "graph",
        "query": qry_tex,
        "doc_id": doc_id,
        "exp_id": 0
    }
    j = send_json(hi_req, hi_url)
    print(json.dumps({
        "key": key,
        "qry": j['qry'],
        "doc": j['doc'],
    }), file=fh)

def print_operands_highlight(key, qry_tex, doc_tex, qmasks, dmasks, fh):
    hi_req = {
        "query_type": "operands",
        "mask0": qmasks[0],
        "mask1": qmasks[1],
        "mask2": qmasks[2],
        "tex": qry_tex
    }
    j_qry = send_json(hi_req, hi_url)
    hi_req = {
        "query_type": "operands",
        "mask0": dmasks[0],
        "mask1": dmasks[1],
        "mask2": dmasks[2],
        "tex": doc_tex
    }
    j_doc = send_json(hi_req, hi_url)
    print(json.dumps({
        "key": key,
        "qry": j_qry['hi_tex'],
        "doc": j_doc['hi_tex'],
    }), file=fh)

doc = {}
print('reading corpus ...')
with open(corpus) as f:
    for line in f:
        fields = line.split()
        tex = ' '.join(fields[1:])
        tex = tex.replace('% ', '')
        doc[fields[0]] = tex

query = {}
print('reading topic ...')
with open(topics) as f:
    for line in f:
        fields = line.split()
        query[fields[0]] = ' '.join(fields[1:])

trecfile = sys.argv[1]
tot_lines = 0
with open(trecfile) as f:
    tot_lines = sum(1 for _ in f)

out_hi_operands = trecfile + '.' + suffix_operands
out_hi_trees    = trecfile + '.' + suffix_trees
out_o = open(out_hi_operands, "w")
out_t = open(out_hi_trees, "w")
with open(trecfile) as f:
    for c, line in enumerate(f):
        print('%u / %u' % (c, tot_lines))
        if limit_line == c:
            break
        line = line.strip('\n')
        # parse line fields
        fields = line.split()
        qry_id = fields[0]
        qry_tex = query[qry_id]
        doc_id, qm, dm = parse_annotat(fields[1])
        doc_nm = fields[2]
        doc_tex = doc[doc_nm]
        run_id = fields[5]
        # print highlight info to stderr/stdout
        key = run_id + '.' + qry_id + '.' + doc_nm
        print_graph_highlight(key, qry_tex, doc_id, out_t)
        print_operands_highlight(key, qry_tex, doc_tex, qm, dm, out_o)

print('closing output files', [out_hi_operands, out_hi_trees])
out_o.close()
out_t.close()
