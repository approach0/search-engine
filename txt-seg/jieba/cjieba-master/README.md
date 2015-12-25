# cjieba [![Build Status](https://travis-ci.org/yanyiwu/cjieba.png?branch=master)](https://travis-ci.org/yanyiwu/cjieba)


其实就是 [CppJieba] 的 C语言 api 接口，
独立出来作为一个仓库的原因和 [libcppjieba] 一样，
都是不想让 [CppJieba] 变太复杂和臃肿而已。

## 用法示例

```
make
./demo
```

## 编译相关

如果编译报关于 `tr1/unordered_map` 的错时，
解决方法是加上 `-std=c++0x` 或者 `-std=c++11` 选项即可，比如：

```
g++ -std=c++11 -DLOGGER_LEVEL=LL_WARN -o c_api.o -c src/c_api.cpp
```

还有就是选项 `-DLOGGER_LEVEL=LL_WARN` 的含义是日志级别设置为警告级别以上才打日志，
如果不设置该选项则会连 DEBUG 级别的日志也打出来。

## 客服

- i@yanyiwu.com

[CppJieba]:http://github.com/yanyiwu/cppjieba
[libcppjieba]:http://github.com/yanyiwu/libcppjieba
