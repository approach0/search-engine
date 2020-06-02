![](https://api.travis-ci.org/approach0/search-engine.svg)
![](http://github-release-version.herokuapp.com/github/approach0/search-engine/release.png)

![](https://raw.githubusercontent.com/approach0/search-engine-docs-eng/master/logo.png)

Approach0 is a math-aware search engine.

Math search can be helpful in Q&A websites: Assume you are fighting a tough
question in your homework, spending so much time on it without any clue.
Yes, you do not simply want an answer, but all you need is some hints.
And spending a lot of time on it without any progress is absolutely a desperate
experience. It would be very helpful if you can search for similar or identical
questions that have been answered on the Q&A websites.

## Online demo
Please visit [https://approach0.xyz/demo](https://approach0.xyz/demo) for a WEB demo.

![](https://github.com/approach0/search-engine-docs-eng/raw/master/img/clip.gif)

## A little history
In 2014, the idea of searching math formulas takes off as a graducate level course project (at University of Delaware).

Later, I am persuaded by my instructor to further do some research work on this, she then became my advisor at that time.

In 2015 summer, [my thesis](https://github.com/tkhost/tkhost.github.io/raw/master/opmes/thesis-ref.pdf) on this topic is submitted.

In 2016, a [math-only search engine prototype](https://github.com/tkhost/tkhost.github.io/raw/master/opmes/ecir2016.pdf) is published in ECIR 2016.

Shortly after, this Github project is created, the goal was to rewrite most of the code and develop a math-aware search engine that can combine both math formula and text keywords into query.

In late 2016, the first rewritten version of math-aware search engine is complete, it is announced in [Meta site of Mathematics StackExchange](https://math.meta.stackexchange.com/questions/24978) and top users on that site have acknowledged the usefulness and also provided some good advices.

In 2017, I go back to China and work at Huawei doing a STB (TV box) project, irrelevant to search engine whatsoever.

In 2017 Fall, with an intuition that a math-aware search engine will provide huge value to many people, I gave up my job and continued working on this topic as a PhD student at [RIT](https://www.cs.rit.edu/~dprl/members.html).
During the first two years of my PhD study, I have kept improving the effectiveness and efficiency of the formula retrieval model.

In 2019, the new model has brought me [my first research full paper](http://ecir2019.org/best-paper-awards/) at ECIR 2019 conference (and a best application paper award!).

In May 2019, the new model has been put online, it has indexed over 1 million posts and there are only 3 search instances running on two low-cost Linode servers.

In early 2020, a paper focusing on efficiency has just been accepted at [ECIR 2020](https://link.springer.com/chapter/10.1007/978-3-030-45439-5_47) conference, this paper shows our system is the first one to produce very effective math search results with practical query runtimes.

## Documentation
Please check out our documentation for technical details:
[https://approach0.xyz/docs](https://approach0.xyz/docs)

## License
MIT

## Updates (May 24, 2019)
Currently, this master branch is inactive, although the research branch has an early version of the new model, it only supports math queries.
The most updated code is closed source, it features very effective math-aware search and further supports text queries.
The new system can search on 1 million documents in real time, hosted by only two low-cost servers.
For information on the most recent development, please email `wxz8033 AT rit.edu` or contact me via WeChat (`hellozhongwei`).
