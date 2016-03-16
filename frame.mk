# list the module names you want to compile
MODULE_NAMES := \
hello hello2 list term-lookup txt-seg wstring tree \
tex-parser

EXT_MODULE_NAMES := cjieba

# list inner module dependencies

# two examples
hello2-module: hello-module
hello-module:

# other dependencies
tex-parser-module: tree-module

wstring-module:

cjieba-extmodule:

tree-module: list-module

list-module:

tree-index-module:

term-lookup-module: wstring-module

txt-seg-module: wstring-module cjieba-extmodule
