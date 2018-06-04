import re

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
	elif tok == "#require":
		after = parse_blk('', '', 0, iterator, '', prev)
		base = ['/* require', after[0], '*/']
		print('include: ', after[0])
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

fname = 'test.dt.c'
with open(fname) as fh:
	content = fh.read()
	print(preprocess(content))
