import sys
sys.path.insert(0, './pya0')

import pya0
tokens = pya0.tokenize("1 + \\frac 1 {n + 1}",
	include_syntatic_literal=True)
for tokenID, tokenType, symbol in tokens:
	print(tokenID, tokenType, symbol)
