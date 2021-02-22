import sys
sys.path.insert(0, './lib')

import pya0
tokens = pya0.lex("a + \\frac b {a + 1} \n a^2 + b^2 = {c}^2")
print(tokens)
