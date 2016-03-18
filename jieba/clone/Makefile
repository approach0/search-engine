all: demo
demo: libjieba.a
	gcc -o demo demo.c -L./ -ljieba -lstdc++ -lm
libjieba.a:
	g++ -DLOGGER_LEVEL=LL_WARN -o jieba.o -c src/jieba.cpp
	ar rs libjieba.a jieba.o 
clean:
	rm -f *.a *.o demo
