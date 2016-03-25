## About
A math-aware search engine.

This project is originated from my previous [math-only search engine prototype](https://github.com/t-k-/opmes).
There are some amazing ideas coming up since that, so a decision is made to rewrite most the code.
This time, however, it has the ambition to become a production-level search engine with improved efficiency and most importantly, to be able to handle queries with both math expressions and general text terms.

This project is still under early development so there is not really much to show here. 

But keep your eyes open and pull request is appreciated!

## Generate module dependency graph.
```
mkdir -p tmp
python3 proj-dep-graph.py > tmp/dep.dot
dot -Tpng tmp/dep.dot > tmp/dep.png
```
