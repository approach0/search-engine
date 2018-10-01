# How to create cache-list
./run/searchd.out -i /home/tk/index-fix-decimal-and-use-latexml -c 0 | grep --line-buffered 'on disk' | awk '{print $4}' | tee cache-list.tmp
