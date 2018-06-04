import re
import os

dependency = dict()
srch_dirs = ['./', '../']

def search_file(fpath):
	path = fpath
	for prefix in srch_dirs:
		if os.path.isfile(prefix + fpath):
			path = prefix + fpath
			break
	# canonicalize the relative path name
	return os.path.relpath(path)

def add_dependency(a, b):
	if a not in dependency:
		dependency[a] = list()
	if b not in dependency[a]:
		dependency[a].append(b)

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

def DFS_add_deptok(a, token):
	if token[0] == '<':
		add_dependency(a, token)
	else:
		# add file 'b' as dependency
		fpath = token.strip('"')
		fpath = search_file(fpath)
		b = '"' + fpath + '"'
		add_dependency(a, b)
		# add dependencies in file 'b'
		dep_toks = parse_deptok(fpath)
		for tok in dep_toks:
			DFS_add_deptok(b, tok)

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
		DFS_add_deptok('_self_', after[0])

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

#fname = 'test.dt.c'

fname = '../search/search.h'
with open(fname) as fh:
	content = fh.read()
	output_body = preprocess(content)
	print(dependency)
	print(output_body)
