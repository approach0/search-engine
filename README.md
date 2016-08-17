![](https://api.travis-ci.org/approach0/search-engine.svg)
[![GitHub release](https://img.shields.io/github/release/approach0/search-engine.svg?maxAge=2592000)]()

![](https://github.com/approach0/search-engine-docs-eng/raw/master/img/clip.gif)

## About
This is a repo for a math-aware search engine. "Math-aware" means
you can add math expression(s) as some of your keywords to have search engine
help you find similar expressions and return those documents that you may find
relevant to your input. In short, a typical search engine plus math search.

Will come up a catchy name for this project someday.
Any suggestions?

BTW. This project needs an icon too! If you are interested to contribute an icon, contact me or send a pull request.

## Use case
Math search can be helpful in math Q&A websites: Assume you are doing a tough
question on your math homework, you type in search box the exact question from
your textbook, after a second (maybe too much) you get all the similar questions
that somebody has already asked about, they are perhaps answered!

## Online demo
[https://approach0.xyz/demo](https://approach0.xyz/demo)

## A little history
The author has been doing a
[math-only search engine prototype](https://github.com/t-k-/opmes)
during his graduate research. Shortly after that prototype is implemented, this
project is created. Its mission is to combine both math-search and fulltext search.
Now I am happy this goal is achieved too! The next step is continuously develop
this project and focus on improving effectiveness and efficiency.

For now only math-aware search is implemented, but these idea can actually be generally
applied to other structural text search such as code, chemistry formula and more.

Technically, this search engine is still based on previously existing and established
fulltext search methods. On top of these, it adds new ideas and algorithms to handle
structural content (see paper
[1](https://github.com/tkhost/tkhost.github.io/raw/master/opmes/thesis-ref.pdf) and
[2](https://github.com/tkhost/tkhost.github.io/raw/master/opmes/ecir2016.pdf))

## Documentation
Please check out our documentation for technical details:
[https://approach0.xyz/docs](https://approach0.xyz/docs)

## License
MIT

## Contact
zhongwei@udel.edu
