## About
This is a repo for a future math-aware search engine.

No worry, "the day after tomorrow" is a temporary name, will come up a more catchy name for this project someday. 

This project is originated from my previous [math-only search engine prototype](https://github.com/t-k-/opmes).
There are some amazing ideas coming up since then, so a decision is made to rewrite most the code.
This time, however, it has the ambition to become a production-level search engine with improved efficiency, and most importantly, to be able to handle queries with both math expressions and general text terms. In another word, to be "math-aware".

This project is still under early development so there is not really much to show here. 
But keep your eyes open and pull request is appreciated!

The meaning behind "the day after tomorrow"? Because reinventing a search engine in C has a long way to go, but as the saying goes:

> Today is hard, tomorrow will be worse, 
> but the day after tomorrow will be sunshine.
>
> (Jack Ma)

## Generate module dependency graph
```
mkdir -p tmp
python3 proj-dep-graph.py > tmp/dep.dot
dot -Tpng tmp/dep.dot > tmp/dep.png
```
