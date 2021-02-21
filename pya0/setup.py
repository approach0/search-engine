from setuptools import setup, Extension

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name = 'pya0',
    version = '0.1',
    author = 'Wei Zhong',
    author_email = "clock126@126.com",
    description = 'Approach Zero Python Interface',
    long_description = long_description,
    long_description_content_type = "text/markdown",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
    ],
    python_requires='>=3.6',
    ext_modules = [
        Extension('pya0',
            sources = ['lexer.py.c', 'main.py.c'],
            include_dirs = [".."],
            libraries=['tex-parser', 'fl'], # order matters
            library_dirs=['../tex-parser/.build']
        )
    ]
)
