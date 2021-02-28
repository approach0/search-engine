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
* `index_writer(ix: index_handler) -> index_writer`
* `writer_close(writer: index_writer) -> None`
* `writer_maintain(writer: index_writer, force: bool) -> bool`
* `writer_flush(writer: index_writer) -> None`
* `writer_add_doc(writer: index_writer, content: str, url: str) -> int`
* `search(ix: index_handler, keywords: list[dict[str, str]], verbose: bool) -> str`

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
$ python3 -m pip3 install --upgrade build # install pip-build tool
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
