from setuptools import setup, Extension

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name = 'pya0',
    version = '0.1.91',
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
    package_data={'': ['pya0/pya0.so', 'pya0/__init__.py']},

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
