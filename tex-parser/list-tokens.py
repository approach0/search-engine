tokens = {}
with open('auto-gen.tmp') as f:
	lines = f.readlines()
	for line in lines:
		fields = line.rstrip().split()
		tok = fields[3]
		sym = fields[1]
		if tok in tokens:
			tokens[tok].append(sym)
		else:
			tokens[tok] = [sym]

#print(tokens)

for tok in tokens.keys():
	print(tok)
	for sym in tokens[tok]:
		tex = sym
		tex = tex.replace("\\\\", "\\")
		tex = tex.replace("\"", "")
		tex = tex.replace("<mat>", "")
		print('\t\t\t', '[imath]%s[/imath]' % tex)
