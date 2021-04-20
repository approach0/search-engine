import sys
sys.path.insert(0, '.')

import pya0

def print_OPT(OPT, level=0):
    nodeID, token, symbol, children = OPT
    space = '   ' * level
    print(f'{space}#{nodeID} {token} {symbol}')
    for rank, c in enumerate(children):
        print_OPT(c, level=level+1)

def test(tex):
    print(f'Parse: {tex}')
    res, OPT = pya0.parse(tex, insert_rank_node=True)
    if res == 'OK':
        print_OPT(OPT)
    else:
        print('Error: ', res)
    print()

# parse a valid TeX
test("a^2 - b^2 = -\\frac{c^3}{2}")

# parse an invalid TeX
test("(a")
