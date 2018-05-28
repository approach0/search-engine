include ../rules.mk
EXCLUDE_SRC := run/toy.dog.c
include ../module.mk

all: #run/toy.out

run/toy.out: .build/toy.main.o

.build/toy.main.o: run/toy.c

run/toy.c:
	./dogcc.out run/toy.dog.c > $@

clean:
	rm -f run/toy.c
	@ echo 'clean'
