import re
import os

#process_file = '../search/search.h'
process_file = './test.dt.c'

dependency = dict()
srch_dirs = ['.', '..']

def search_file(prefix, fpath):
	if prefix == '':
		prefix = '.'
	for srch_dir in srch_dirs:
		loc = prefix + '/' + srch_dir + '/' + fpath;
		if os.path.isfile(loc):
			return loc
	# canonicalize the relative path name
	return None

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

def DFS_add_deptok(prefix, a, token):
	if token[0] == '<':
		add_dependency(a, token)
	else:
		# add file 'b' as dependency
		fpath = token.strip('"')
		fpath = os.path.relpath(fpath)
		b = '"' + fpath + '"'
		if not add_dependency(a, b):
			return # added before, stop here
		# add dependencies in file 'b'
		loc = search_file(prefix, fpath)
		if loc is None:
			print('cannot find %s from prefix %s' % (
				fpath, prefix))
			return
		dep_toks = parse_deptok(loc)
		for tok in dep_toks:
			DFS_add_deptok(prefix, b, tok)

def parse_blk(begin, end, stack, iterator, sep, prev):
	try:
		tok = next(iterator)
	except StopIteration:
		return [];
	base = [tok]
	if tok == begin:
		if stack == 0: base = []
		stack += 1
	elif tok == end:
		stack -= 1
		if stack == 0: return []
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
		base = ['/* require', after[0], '*/']
		global process_file
		process_file = os.path.relpath(process_file)
		fname = os.path.basename(process_file)
		dname = os.path.dirname(process_file)
		# print('include:', dname, fname)
		DFS_add_deptok(dname, fname, after[0])

		return base + after[1:]

	prev = tok
	return base + parse_blk(begin, end, stack, iterator, sep, prev)

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
	print(dependency)
	print(output_body)
