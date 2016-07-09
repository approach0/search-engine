.PHONY: all lib clean

all: lib

lib: fork/build
	make -C fork distro

fork/build:
	(cd fork && ./configure)

clean:
	rm -rf fork/build
	rm -f fork/Makefile
	@ echo 'clean'
