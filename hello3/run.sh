#!/bin/sh
set -e
python3 ../proj-preprocessor.py -I . -I .. ./test.dt.c  > test.c
gcc test.c -std=c99 -I . -I .. -L ../strmap/.build -lstrmap -L ../sds/.build -lsds

./a.out
rm a.out
set +e
