# cjieba 

[![Build Status](https://travis-ci.org/yanyiwu/cjieba.png?branch=master)](https://travis-ci.org/yanyiwu/cjieba)
[![Author](https://img.shields.io/badge/author-@yanyiwu-blue.svg?style=flat)](http://yanyiwu.com/) 
[![Performance](https://img.shields.io/badge/performance-excellent-brightgreen.svg?style=flat)](http://yanyiwu.com/work/2015/06/14/jieba-series-performance-test.html) 
[![License](https://img.shields.io/badge/license-MIT-yellow.svg?style=flat)](http://yanyiwu.mit-license.org)

其实就是 [CppJieba] 的 C语言 api 接口，
独立出来作为一个仓库的原因是不想让 [CppJieba] 变太复杂和臃肿而已。

## 用法示例

```
make
./demo
```

## 编译相关

如果编译报关于 `tr1/unordered_map` 的错时，
解决方法是加上 `-std=c++0x` 或者 `-std=c++11` 选项即可，比如：

```
g++ -std=c++11 -DLOGGING_LEVEL=LL_WARNING -o c_api.o -c src/c_api.cpp
```

还有就是选项 `-DLOGGING_LEVEL=LL_WARNING` 的含义是日志级别设置为警告级别以上才打日志，
如果不设置该选项则会连 DEBUG 或者 INFO 级别的日志也打出来。

## 客服

- i@yanyiwu.com

[CppJieba]:http://github.com/yanyiwu/cppjieba
[libcppjieba]:http://github.com/yanyiwu/libcppjieba


[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/yanyiwu/cjieba/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

