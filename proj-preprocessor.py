import re
import os
import getopt, sys

process_file = None
additional_srch_dirs = []
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
	topo = topo[1:] # exclude current file.
	lines = []
	if topo is None:
		return None
	for h in reversed(topo):
		if h[0] == '<':
			lines.append(' '.join(['#include', h]))
		else:
			# empty line
			lines.append('')
			# actual #include
			line = ' '.join(['#include', '"' + h + '"'])
			lines.append(line)
			# comment on dependencies
			if h in dep:
				line = "/* depends on: "
				for hh in dep[h]:
					line += hh + ' '
				line += "*/"
				lines.append(line)
			# empty line
			lines.append('')
	# empty line
	lines.append('')
	if len(lines):
		return '\n'.join(lines)
	else:
		return None

def parse_blk(begin, end, stack, iterator, ignores, prev):
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
		elif ignores is not None and tok in ignores:
			base = [] # skip ignored tokens
		elif tok == "foreach":
			args = parse_blk('(', ')', 0, iterator, [',', ' '], prev)
			block = parse_blk('{', '}', 0, iterator, None, prev)
			emit = (
				"if (!%s_empty(%s)) "
				"{ struct %s_iterator %s = %s_iterator(%s); do { "
				"%s } while (%s_iter_next(%s, & %s)); }"
				) % (
					args[1], args[2],
					args[1], args[0], args[1], args[2],
					hacky_join(block), args[1], args[2], args[0]
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
			for i in range(len(after)):
				if after[i] != ' ':
					base = ['/* require', after[0], '*/'] + after[i + 1:]
					DFS_add_dep(process_file, after[i])
					break
		if tok != ' ':
			prev = tok
		ret_tokens.extend(base)
	return ret_tokens

def hacky_join(toks):
	joined = ''
	for i, tok in enumerate(toks):
		# '' represents a backspace here
		if i + 1 < len(toks) and toks[i + 1] == '':
			continue
		elif tok == '':
			continue
		joined += tok
	return joined

def preprocess(content):
	toks = re.split('(\[|\]|\(|\)|,| |\t|\n)', content)
	toks = [tok for tok in toks if tok != '']
	toks = parse_blk('', '', 0, iter(toks), '', None)
	return hacky_join(toks)

def help(arg0):
	print('DESCRIPTION: Duct-taped C preprocessor.' \
	      '\n' \
	      'SYNOPSIS:\n' \
	      '%s [-h | --help] ' \
	      '[-I <search directory>] ' \
	      '<input file> ' \
	      '\n' \
	      % (arg0))
	sys.exit(1)

def main():
	global process_file
	global additional_srch_dirs

	cmd = sys.argv[0]
	args = sys.argv[1:]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hI:", ['help'])
	except:
		help(cmd)

	if len(args) == 0:
		help(cmd)

	for opt, arg in opts:
		if opt in ("-h", "--help"):
			help(cmd)
			break
		elif opt in ("-I"):
			additional_srch_dirs.append(arg)

	process_file = args[0]

	with open(process_file) as fh:
		content = fh.read()
		output_body = preprocess(content)
		output_head = headers(dependency)
		if output_head is not None:
			print(output_head)
		print(output_body)

if __name__ == "__main__":
	main()
