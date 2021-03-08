import sys
sys.path.insert(0, './pya0')

import os
import time
import pya0
import json

index_path = "tmp" # output index path

if not os.path.exists(index_path):
    print('ERROR: No such index directory.')
    quit()

HOME = os.getenv("HOME")
ix = pya0.index_open(index_path, option="r")

print('Searching ...')

JSON = pya0.search(ix, [
    { 'keyword': 'b^2', 'type': 'tex'},
    { 'keyword': 'induction', 'type': 'term'}
], verbose = False, topk= 10, trec_output="./trec.out")
results = json.loads(JSON)
print(json.dumps(results, indent=4))

print('Testing fallback parser ...')

pya0.use_fallback_parser(True)
JSON = pya0.search(ix, [ { 'keyword': '(b^2', 'type': 'tex'} ], verbose=True)
results = json.loads(JSON)
print(json.dumps(results, indent=4))

pya0.index_close(ix)
