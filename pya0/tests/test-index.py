import sys
sys.path.insert(0, './lib')

import os
import time
import pya0

index_path = "tmp"

if not os.path.exists(index_path):
    print('Create a new index')
    ix = pya0.index_open(index_path)

    writer = pya0.index_writer(ix)
    if pya0.writer_maintain(writer, force=True):
        print('index merged')
    pya0.writer_flush(writer);
    pya0.close_writer(writer)
else:
    print('Open with read-only mode')
    HOME = os.getenv("HOME")
    ix = pya0.index_open(index_path,
        option="r", segment_dict=f"{HOME}/cppjieba/dict")

pya0.index_close(ix)
