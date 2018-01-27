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
    return len(q) <= len(d) and q[0] == d[0] and q[1] == d[1]

def align(t_q, t_d, qd_map, path_map):
    dq_map = {v:k for k,v in qd_map.items()}
    cnt = 0
    for i, _ in enumerate(t_q[0]):
        if t_q[1][i] != t_d[1][i]:
            break
        n_q, n_d = t_q[0][i], t_d[0][i]
        if n_q in qd_map:
            if qd_map[n_q] != n_d:
                return 0
        elif n_d in dq_map:
            if dq_map[n_d] != n_q:
                return 0
        else:
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

def similarity(query, doc, hitlist):
    queue = [(-1, -1, {}, {}, 0)]
    last_i = 0
    level_hit = False
    level_max = 0
    best_map = {}
    best_score = 0
    while len(queue):
        i, j, node_map, path_map, score = queue.pop(0)
        if i != last_i:
            level_max = 0
            level_hit = False
        last_i = i
        if i != -1:
            q = query[i]
            d = doc[j]
            copy_node_map = dict(node_map)
            cnt = align(q, d, node_map, path_map)
            if cnt > 2:
                level_hit = True
                score += cnt
                # print(score, len(node_map))
                if score > level_max:
                    level_max = score
                    best_map = path_map
                    best_score = score
            else:
                if level_hit:
                    continue
                else:
                    node_map = copy_node_map;

        if i + 1 < len(query):
            for t in hitlist[i + 1]:
                n = (i + 1, t, dict(node_map), dict(path_map), score)
                queue.append(n);
    return (best_score, best_map)

query = parse_file(arg1)
doc   = parse_file(arg2)
hitlist = get_hitlist(query, doc)
score, align = similarity(query, doc, hitlist)

print(query, end='\n')
print(doc, end='\n')
print(*hitlist, sep='\n', end="\n")
print(align)
print('Score: %d, %d/%d' % (score, len(align), len(hitlist)))
