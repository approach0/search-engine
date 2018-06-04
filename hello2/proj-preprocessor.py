import re
import os

process_file = '../searchd/run/searchd.c'

# process_file = './test.dt.c'
# srch_dirs = ['.']

#process_file = '../search/search.h'
srch_dirs = ['.', '..', '../..']

dependency = dict()

def add_dependency(a, b):
	if a not in dependency:
		dependency[a] = list()
		return True
	if b not in dependency[a]:
		dependency[a].append(b)
		return True
	return False

def parse_deptok(fpath):
	toks = []
	if not os.path.isfile(fpath):
		return []
	with open(fpath) as fh:
		content = fh.read()
		tokens = content.split()
	extract_next = False
	for tok in tokens:
		if tok == '#include' or tok == '#require':
			extract_next = True
		elif extract_next:
			toks.append(tok)
			extract_next = False
	return toks

def search_file(prefix, cd, fpath):
	for srch_dir in srch_dirs:
		loc = prefix + '/' + cd + '/' + srch_dir + '/' + fpath
		if os.path.isfile(loc):
			rpath = cd + '/' + srch_dir + '/' + fpath
			return (loc, os.path.relpath(rpath))
	return (None, None)

def DFS_add_deptok(prefix, cd, a, b):
	if prefix == '':
		prefix = '.'
	if b[0] == '<':
		add_dependency(a, b)
	else:
		# add file 'b' as dependency
		b = b.strip('"')
		loc, b = search_file(prefix, cd, b)
		if loc is None:
			print('cannot find %s from prefix %s and %s' %
			      (b, prefix, cd))
			return
		if not add_dependency(a, b):
			return # added before, stop here
		# add dependencies in file 'b'
		dep_toks = parse_deptok(loc)
		for c in dep_toks:
			cd = os.path.dirname(b)
			if cd == '': cd = '.'
			DFS_add_deptok(prefix, cd, b, c)

def count(incoming_edges, dep, node, incr):
	if node in incoming_edges:
		incoming_edges[node] += incr
	else:
		incoming_edges[node] = incr

def DFS_detect_cycle(stack, cur, dep):
	if cur not in dep: return False
	for d in dep[cur]:
		if d == stack[0]:
			print(stack)
			return True
		else:
			newstack = [x for x in stack]
			newstack.append(d)
			return DFS_detect_cycle(newstack, d, dep)
	return False

def topological_order(dep):
	incoming_edges = dict()
	for n in dep:
		count(incoming_edges, dep, n, 0)
		for d in dep[n]:
			count(incoming_edges, dep, d, 1)
	topo = list()
	S = list()
	this_file_nm = os.path.basename(process_file)
	if incoming_edges[this_file_nm] == 0:
		S.append(this_file_nm)
	while len(S):
		v = S.pop()
		topo.append(v)
		if v not in dep: continue
		for vn in dep[v]:
			if incoming_edges[vn] > 0:
				incoming_edges[vn] -= 1
			if incoming_edges[vn] == 0:
				S.append(vn)
	for n in incoming_edges:
		if incoming_edges[n] > 0:
			if DFS_detect_cycle([n], n, dep):
				return None
	return topo

def headers(dep):
	topo = topological_order(dep)
	lines = []
	if topo is None:
		return None
	for h in reversed(topo):
		if h[0] == '<':
			lines.append(' '.join(['#include', h]))
		else:
			line = ' '.join(['#include', '"' + h + '"'])
			if h in dep:
				line += " /* "
				for hh in dep[h]:
					line += hh + ' '
				line += "*/"
			lines.append(line)
	return '\n'.join(lines)

def parse_blk(begin, end, stack, iterator, sep):
	ret_tokens = []
	while True:
		try:
			tok = next(iterator)
		except StopIteration:
			break
		base = [tok]
		if tok == begin:
			if stack == 0: base = []
			stack += 1
		elif tok == end:
			stack -= 1
			if stack == 0: break
		elif tok == sep:
			base = [] # skip separator

		if tok == "foreach":
			args = parse_blk('(', ')', 0, iterator, ',')
			block = parse_blk('{', '}', 0, iterator, None)
			emit = (
				"{ struct %s_iterator %s = %s_iterator(%s); do {\n"
				"%s } while (%s_next(%s, & %s)); }"
				) % (
					args[1], args[0], args[1], args[2],
					' '.join(block), args[0], args[2], args[0]
				)
			base = [emit]
		elif tok == "[":
			core = parse_blk('[', ']', 1, iterator, None)
			if len(core) > 0 and core[0][0] == '"':
				keystr = ' '.join(core)
				emit = "(*strmap_val_ptr(%s, %s))" % (prev, keystr)
				base = ['', emit]
			else:
				base = ['['] + core + [']']
		elif tok == "#require" or tok == "#include":
			after = parse_blk('', '', 0, iterator, '')
			base = ['/* require', after[0], '*/'] + after[1:]
			global process_file
			process_file = os.path.relpath(process_file)
			fname = os.path.basename(process_file)
			prefix = os.path.dirname(process_file)
			DFS_add_deptok(prefix, '.', fname, after[0])
		ret_tokens.extend(base)
	return ret_tokens

def hacky_join(toks):
	joined = ''
	for i, tok in enumerate(toks):
		if i + 1 == len(toks) and tok == '\n':
			continue
		elif i + 1 < len(toks) and toks[i + 1] == '':
			continue
		elif tok == '':
			continue
		elif tok == '\n':
			joined += tok
		else:
			joined += tok + ' '
	return joined

def preprocess(content):
	toks = re.split('(\[|\]|\(|\)|,| |\t|\n)', content)
	toks = [tok for tok in toks if tok != ' ']
	toks = [tok for tok in toks if tok != '']
	toks = parse_blk('', '', 0, iter(toks), '')
	return hacky_join(toks)

with open(process_file) as fh:
	content = fh.read()
	output_body = preprocess(content)
	output_head = headers(dependency)
	if output_head is None:
		print('cycle detected!')
	else:
		print(output_head)
		print(output_body)
