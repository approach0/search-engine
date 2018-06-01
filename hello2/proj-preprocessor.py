import re

fname = 'print-hello-world.dt.c'

def parse(scope_open, scope_close, iterator, **kwargs):
	stack_num = 1
	scope_ele = []
	sep = kwargs["sep"] if "sep" in kwargs else None
	while True:
		try:
			tok = next(iterator)
		except StopIteration:
			break
		if tok == ' ' or tok == '':
			continue
		elif tok == scope_open:
			stack_num += 1
			scope_ele.append(scope_open)
		elif tok == scope_close:
			stack_num -= 1
			# break immediately if stack is empty
			if stack_num == 0:
				break
			else:
				scope_ele.append(scope_close)
		elif tok != sep:
			scope_ele.append(tok)
	return scope_ele

def foreach_headler(context, output):
	c = context
	tok = c["curr_tok"]
	iterator = c["iterator"]
	if tok == "foreach":
		c['scope_nm'] = "foreach_args"
	elif c['scope_nm'] == "foreach_args" and tok == "(":
		c["args"] = parse('(', ')', iterator, sep=',')
		c['scope_nm'] = "foreach_block"
	elif c['scope_nm'] == "foreach_block" and tok == "{":
		block = parse('{', '}', iterator)
		args = c["args"]
		emit = (
			"{ struct %s_iterator %s = %s_iterator(%s); do {\n"
			"%s } while (%s_next(%s, & %s)); } \n"
			) % (
				args[1], args[0], args[1], args[2],
				' '.join(block), args[0], args[2], args[0]
			);
		output.append(emit)
		c['scope_nm'] = None

# def dict_headler(context, output):
# 	c = context
# 	tok = c["curr_tok"]
# 	iterator = c["iterator"]
# 	if tok == "[":

#
# Main
#

with open(fname) as fh:
	content = fh.read()
	toks = re.split('(\[|\]|\(|\)|,| |\t|\n)', content)

context  = {
	"scope_nm": None,
	"prev_tok": None,
	"curr_tok": None,
	"iterator": iter(toks)
	"stack": iter(toks)
}
output   = []
for tok in context["iterator"]:
	if tok == ' ' or tok == '':
		continue
	elif context['scope_nm'] is None:
		output.append(tok)
	# handle FOREACH statement pattern
	context["curr_tok"] = tok
	foreach_headler(context, output)
	context["prev_tok"] = tok

print(' '.join(output))
