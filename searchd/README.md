# How to create prophet cache list
```sh
$ rm -f ./cache-list.txt
$ ./run/searchd.out -i /path/to/index -c 0 | grep --line-buffered 'on disk' | awk '{print $4; fflush(stdout)}' | tee ./cache-list.txt
```
