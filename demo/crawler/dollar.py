def replace_dollar_tex(s):
	l = len(s)
	i, j, stack = 0, 0, 0
	new_txt = ''
	while i < l:
		if s[i] == "\\" and (i + 1) < l:
			# skip if it is escaped dollar
			if s[i + 1] == '$':
				i += 1
				new_txt += '\\$'
		elif s[i] == '$':
			if stack == 0: # first open dollar
				stack = 1
				j = i + 1
			elif stack == 1: # second dollar
				if i == j:
					# consecutive dollar
					# (second open dollar)
					stack = 2
					j = i + 1
				else:
					# non-consecutive dollar
					# (close dollar)
					stack = 0
					# print('single: %s' % s[j:i])
					new_txt += '[imath]%s[/imath]' % s[j:i]
			else: # stack == 2
				# first close dollar
				stack = 0
				# print('double: %s' % s[j:i])
				new_txt += '[imath]%s[/imath]' % s[j:i]
				# skip the second close dollar
				i += 1
		elif stack == 0:
			# non-escaped and non enclosed characters
			new_txt += s[i]
		i += 1
	return new_txt

# print(replace_dollar_tex('given $Y=f(x)$ and $$f \in P$$'))
