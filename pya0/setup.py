from setuptools import setup, Extension

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

import os
def copy_files():
    dir_path = 'topics-and-qrels'
    base_dir = os.path.join('./pya0', dir_path)
    for (dirpath, dirnames, files) in os.walk(base_dir):
        for f in files:
            yield os.path.join(dirpath.split('/', 1)[1], f)

setup(
    name = 'pya0',
    version = '0.2.0',
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
    packages=[''],
    package_data={
        '': ['pya0', 'pya0/*'] + [f for f in copy_files()],
    },

    native_module = [
        Extension(
            name='',
            sources = ['main.py.c'],
            include_dirs = ["."],
            libraries=['event', 'mpi', 'stdc++', 'm', 'pthread', 'fl', 'rt', 'xml2', 'z'], # order matters
            library_dirs=[]
        )
    ]
)
