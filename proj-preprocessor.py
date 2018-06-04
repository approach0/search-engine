import re
import os

process_file = '../searchd/run/searchd.c'
process_file = './test.dt'
additional_srch_dirs = ['.', '..']

dependency = dict()

def add_dependency(a, b):
	if a not in dependency:
		dependency[a] = list()
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
		if tok == '#require' or tok == '#include':
			extract_next = True
		elif extract_next:
			toks.append(tok)
			extract_next = False
	return toks

def search_file_from(a, b):
	# first search relatively to the dir of 'a'
	first_srch_dir = os.path.dirname(a)
	if first_srch_dir == '':
		first_srch_dir = '.'
	# append additional search dirs specified
	srch_dirs = [first_srch_dir]
	srch_dirs.extend(additional_srch_dirs)
	# search 'b' file in above order
	for prefix in srch_dirs:
		loc = prefix + '/' + b
		if os.path.isfile(loc):
			return os.path.relpath(loc)
	return None

def DFS_add_dep(a, b):
	# canonicalize path
	a = os.path.relpath(a)
	# depending on the form <*.h> or "*.h"
	if b[0] == '<':
		add_dependency(a, b)
		return
	elif b[0] == '"':
		b = b.strip('"')
	# add file 'b' as dependency
	b_rpath = search_file_from(a, b)
	if b_rpath is None:
		print('cannot find %s included in %s' % (b, a))
		return
	if not add_dependency(a, b_rpath):
		return # added before, stop here
	# add dependencies in file 'b'
	dep_toks = parse_deptok(b_rpath)
	for c in dep_toks:
		DFS_add_dep(b_rpath, c)

def count(incoming_edges, dep, node, incr):
	if node in incoming_edges:
		incoming_edges[node] += incr
	else:
		incoming_edges[node] = incr

def DFS_detect_cycle(stack, cur, dep):
	if cur not in dep: return False
	for d in dep[cur]:
		if d == stack[0]:
			print('#pragma message "CYCLE: %s"' % stack)
			print('#error "cycle detected!"')
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
	this_file = os.path.relpath(process_file)
	if this_file in incoming_edges:
		if incoming_edges[this_file] == 0:
			S.append(this_file)
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
			lines.append(line)
			if h in dep:
				line = "/* deps: "
				for hh in dep[h]:
					line += hh + ' '
				line += "*/"
				lines.append(line)
		lines.append('\n')
	if len(lines):
		return '\n'.join(lines)
	else:
		return None

def parse_blk(begin, end, stack, iterator, sep, prev):
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
			args = parse_blk('(', ')', 0, iterator, ',', prev)
			block = parse_blk('{', '}', 0, iterator, None, prev)
			emit = (
				"{ struct %s_iterator %s = %s_iterator(%s); do {\n"
				"%s } while (%s_next(%s, & %s)); }"
				) % (
					args[1], args[0], args[1], args[2],
					' '.join(block), args[0], args[2], args[0]
				)
			base = [emit]
		elif tok == "[":
			core = parse_blk('[', ']', 1, iterator, None, prev)
			if len(core) > 0 and core[0][0] == '"':
				keystr = ' '.join(core)
				emit = "(*strmap_val_ptr(%s, %s))" % (prev, keystr)
				base = ['', emit]
			else:
				base = ['['] + core + [']']
		elif tok == "#require" or tok == "#include":
			after = parse_blk('', '', 0, iterator, '', prev)
			base = ['/* require', after[0], '*/'] + after[1:]
			DFS_add_dep(process_file, after[0])
		prev = tok
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
	toks = parse_blk('', '', 0, iter(toks), '', None)
	return hacky_join(toks)

with open(process_file) as fh:
	content = fh.read()
	output_body = preprocess(content)
	output_head = headers(dependency)
	if output_head is not None:
		print(output_head)
	print(output_body)
