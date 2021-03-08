from setuptools import setup, Extension

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name = 'pya0',
    version = '0.1.3',
    author = 'Wei Zhong',
    author_email = "clock126@126.com",
    description = 'Approach Zero Python Interface',
    long_description = long_description,
    long_description_content_type = "text/markdown",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.5',
    ext_modules = [
        Extension('pya0',
            sources = ['lexer.py.c', 'indexer.py.c', 'searcher.py.c', 'main.py.c'],
            include_dirs = ["..", "."],
            libraries=['searchd', 'search-v3', 'indices-v3', 'tex-parser', 'fl', 'xml2', 'z', 'stdc++'], # order matters
            library_dirs=['../tex-parser/.build', '../indices-v3/.build', '../search-v3/.build', '../searchd/.build']
        )
    ]
)
