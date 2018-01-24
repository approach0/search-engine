#!/usr/bin/python3
import sys

if len(sys.argv) < 3:
    print('not enough args.')
    quit()

arg1 = sys.argv[1]
arg2 = sys.argv[2]

def parse_file(filepath):
    lines = open(filepath).read().splitlines()
    inputs = [string.split() for string in lines]
    for idx, row in enumerate(inputs):
        l = len(row) // 2
        ids, toks = row[:l], row[l:]
        ids = [int(x) for x in ids]
        inputs[idx] = (ids, toks)
    return inputs

def hit(q, d):
    return len(q) <= len(d) and q[0] == d[0]

def align(t_q, t_d, qd_map, path_map):
    dq_map = {v:k for k,v in qd_map.items()}
    cnt = 0
    for i, _ in enumerate(t_q[0]):
        if t_q[1][i] != t_d[1][i]:
            break
        n_q, n_d = t_q[0][i], t_d[0][i]
        if (n_q in qd_map and qd_map[n_q] != n_d) or (
            n_d in dq_map and dq_map[n_d] != n_q):
            return 0
        qd_map[n_q] = n_d
        dq_map[n_d] = n_q
        cnt += 1
    path_map[t_q[0][0]] = t_d[0][0]
    return cnt

def get_hitlist(query, doc):
    hitlist = []
    for tuple_q in query:
        hits = []
        for idx, tuple_d in enumerate(doc):
            if hit(tuple_q[1], tuple_d[1]):
                hits.append(idx)
        hitlist.append(hits)
    return hitlist

query = parse_file(arg1)
doc   = parse_file(arg2)
hitlist = get_hitlist(query, doc)

print(query)
print('')
print(doc)
print('')
print(*hitlist, sep='\n')
print('')
 
queue = [(-1, -1, {}, {})]
level_max = 0
last_i = 0
best = {}
while len(queue):
    i, j, node_map, path_map = queue.pop(0)
    if i != last_i:
        level_max = 0
    last_i = i
    if i != -1:
        q = query[i]
        d = doc[j]
        res = align(q, d, node_map, path_map)
        # print(i, j, path_map)
        if res > 2:
            level_max = 1
            best = path_map
        elif level_max > 0:
            continue
    if i + 1 < len(query):
        for t in hitlist[i + 1]:
            n = (i + 1, t, dict(node_map), dict(path_map))
            queue.append(n);
print(best)
