import sys
sys.path.insert(0, './pya0')

import os
import time
import pya0

index_path = "tmp" # output index path

if not os.path.exists(index_path):
    print('Testing index creating ...')
    ix = pya0.index_open(index_path)

    writer = pya0.index_writer(ix)

    pya0.writer_add_doc(
        writer,
        "[imath]x^2 + y^2 = z^2[/imath] is called pythagreon therom"
    )
    pya0.writer_flush(writer);

    pya0.writer_add_doc(
        writer,
        content="prove inequality by induction: " +
        "[imath] (a + b)^n \geq a^n + b^n [/imath]",
        url="https://math.stackexchange.com/questions/2528544"
    )
    pya0.writer_flush(writer);

    if pya0.writer_maintain(writer, force=True):
        print('index merged')

    pya0.writer_close(writer)
else:
    print('Testing read-only mode, seg_dict loading and index caching')
    HOME = os.getenv("HOME")
    ix = pya0.index_open(index_path,
        option="r", segment_dict=f"{HOME}/cppjieba/dict")

    pya0.index_memcache(ix, term_cache = 3, math_cache = 5) # in MB
    print(pya0.index_lookup_doc(ix, 2)) # test doc raw content lookup
    pya0.index_print_summary(ix)

pya0.index_close(ix)
