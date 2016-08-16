.PHONY: all lib clean

all: lib

lib: fork/Makefile
	make -C fork distro

fork/Makefile:
	(cd fork && ./configure)

clean:
	rm -rf fork/build
	rm -f fork/Makefile
	@ echo 'clean'
