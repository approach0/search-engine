## About
**pya0** is a Python wrapper for Approach Zero search engine.
It provides Python interface to make the search engine core easy to play with.

### Install Necessary Dependencies (Ubuntu)
```sh
$ sudo apt install build-essential python-dev
$ make
```

### Quick Start
Install `pya0` using pip
```sh
$ sudo pip3 install --upgrade pya0
```

Test a simple math token scanner:
```py
import pya0
lst = pya0.lex('\\lim_{n \\to \\infty} (1 + 1/n)^n')
print(lst)
```
Result:
```
[(269, 'LIM', 'lim'), (274, 'SUBSCRIPT', 'subscript'), (260, 'VAR', "normal`n'"), (270, 'ARROW', 'to'), (260, 'INFTY', 'infty'), (259, 'ONE', "`1'"), (261, 'ADD', 'plus'), (259, 'ONE', "`1'"), (264, 'FRAC', 'frac'), (260, 'VAR', "normal`n'"), (275, 'SUPSCRIPT', 'supscript'), (260, 'VAR', "normal`n'")]
```

Refer to `tests/` directory for more complete example usages.

### Supported Interfaces
* `lex(TeX: str) -> list[tuple(tokID, token, symbol)]`
* `index_open(index_path: str, option: str, segment_dict: str) -> index_handler`
* `index_close(ix: index_handler) -> None`
* `index_memcache(ix: index_handler, term_cache: int, math_cache: int) -> None`
* `index_print_summary(ix: index_handler) -> None`
* `index_lookup_doc(ix: index_handler, docid: int) -> tuple(str, str)`
* `index_writer(ix: index_handler) -> index_writer`
* `writer_close(writer: index_writer) -> None`
* `writer_maintain(writer: index_writer, force: bool) -> bool`
* `writer_flush(writer: index_writer) -> None`
* `writer_add_doc(writer: index_writer, content: str, url: str) -> int`
* `search(ix: index_handler, keywords: list[dict[str, str]], verbose: bool, topk: int, trec_output: str) -> str`

(`lex` function can be useful to train a RNN and predict TeX tokens)

### Local Build and Testing
Ensure to include and prioritize local dist:
```py
import sys
sys.path.insert(0, './lib')
```
then run some test case, for example:
```sh
$ python3 tests/test-lexer.py
```

### Packaging
Build and install package locally (for testing):
```sh
$ make clean
$ sudo python3 setup.py install
```
then, you can import as library from system path:
```py
import pya0
print(dir(pya0))
```

Create a `pip` distribution package:
```sh
$ python3 -m pip install --upgrade build # install pip-build tool
$ sudo python3 -m build
```

### Upload to Pip
Install `twine`
```sh
$ sudo apt install rustc libssl-dev libffi-dev
$ sudo python3 -m pip install setuptools_rust
$ sudo python3 -m pip install --user --upgrade twine
```

Upload package in `dist` directory
```sh
$ python3 -m twine upload --repository pypi dist/*
```
(use username `__token__` and your created token on `https://pypi.org`)

Here if shared library is not manylinux API, we will need to use Docker to build and fix a wheel to support as many as Linux platform as possible.

Install Docker:
```sh
apt-get update
which docker || curl -fsSL https://get.docker.com -o get-docker.sh
which docker || sh get-docker.sh
```

Pull and run a `Debian x86_64` image `quay.io/pypa/manylinux_2_24_x86_64`.
Inside docker container, build pya0 as instructed above, so that you have a linux wheel, e.g., `./dist/pya0-0.1-cp35-cp35m-linux_x86_64.whl`.

Then inspect the wheel:
```sh
# auditwheel show ./dist/pya0-0.1*.whl

pya0-0.1-cp35-cp35m-linux_x86_64.whl is consistent with the following
platform tag: "linux_x86_64".

The wheel references external versioned symbols in these system-
provided shared libraries: libgcc_s.so.1 with versions {'GCC_3.0'},
libz.so.1 with versions {'ZLIB_1.2.0', 'ZLIB_1.2.3.3',
'ZLIB_1.2.2.3'}, libstdc++.so.6 with versions {'GLIBCXX_3.4.10',
'GLIBCXX_3.4.11', 'GLIBCXX_3.4.21', 'GLIBCXX_3.4.15', 'CXXABI_1.3',
'CXXABI_1.3.8', 'GLIBCXX_3.4', 'CXXABI_1.3.9', 'GLIBCXX_3.4.9',
'CXXABI_1.3.1', 'GLIBCXX_3.4.20'}, libpthread.so.0 with versions
{'GLIBC_2.2.5', 'GLIBC_2.3.2', 'GLIBC_2.3.3'}, libc.so.6 with versions
{'GLIBC_2.7', 'GLIBC_2.17', 'GLIBC_2.3.4', 'GLIBC_2.15', 'GLIBC_2.3',
'GLIBC_2.3.2', 'GLIBC_2.4', 'GLIBC_2.22', 'GLIBC_2.2.5',
'GLIBC_2.14'}, libdl.so.2 with versions {'GLIBC_2.2.5'}, libm.so.6
with versions {'GLIBC_2.2.5'}, liblzma.so.5 with versions {'XZ_5.0'}

This constrains the platform tag to "manylinux_2_24_x86_64". In order
to achieve a more compatible tag, you would need to recompile a new
wheel from source on a system with earlier versions of these
libraries, such as a recent manylinux image.
```
the `auditwheel` suggests to use platform `manylinux_2_24_x86_64`.

Fix it to that platform:
```sh
# auditwheel repair ./dist/pya0-0.1-cp35-cp35m-linux_x86_64.whl --plat manylinux_2_24_x86_64 -w ./wheelhouse
INFO:auditwheel.main_repair:Repairing pya0-0.1-cp35-cp35m-linux_x86_64.whl
INFO:auditwheel.wheeltools:Previous filename tags: linux_x86_64
INFO:auditwheel.wheeltools:New filename tags: manylinux_2_24_x86_64
INFO:auditwheel.wheeltools:Previous WHEEL info tags: cp35-cp35m-linux_x86_64
INFO:auditwheel.wheeltools:New WHEEL info tags: cp35-cp35m-manylinux_2_24_x86_64
INFO:auditwheel.main_repair:
Fixed-up wheel written to /io/pya0/wheelhouse/pya0-0.1-cp35-cp35m-manylinux_2_24_x86_64.whl
```

Then you should be able to upload to PIP:
```sh
# python3 -m twine upload --repository pypi wheelhouse/pya0-0.1-cp35-cp35m-manylinux_2_24_x86_64.whl
```
