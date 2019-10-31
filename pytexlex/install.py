from distutils.core import setup, Extension

setup(
    name = 'texlex',
    version = '1.0',
    description = 'texlex package',
    ext_modules = [Extension('texlex', sources = ['texlex.py.c'])]
)
