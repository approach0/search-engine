# How to create prophet cache list
./run/searchd.out -i /path/to/index -c 0 | grep --line-buffered 'on disk' | awk '{print $4}' | tee ./cache-list.txt
