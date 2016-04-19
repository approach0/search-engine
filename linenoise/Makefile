AROBJS := fork/linenoise.o
include ../rules.mk

all: lib

lib: fork/liblinenoise.a
	make -C fork

fork/liblinenoise.a: $(AROBJS)
	$(AR)

clean:
	make -C fork clean
	@ echo 'clean'
