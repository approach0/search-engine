AROBJS := fork/linenoise.o
include ../rules.mk

all: lib

lib: fork/liblinenoise.a
	$(MAKE) -C fork

fork/liblinenoise.a: $(AROBJS)
	$(COLOR_AR)
	$(strip $(GEN_LIB))

clean:
	$(MAKE) -C fork clean
	@ echo 'clean'
