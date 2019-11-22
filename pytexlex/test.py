import sys
sys.path.append('./build')

import texlex
texlex.open('./test.txt')

while True:
    lex_tok, o_tok, o_sym = texlex.next()
    if not lex_tok:
        break

    print(lex_tok, texlex.trans(), end=" ")
    print("OPT:", o_tok, o_sym)

texlex.close()
