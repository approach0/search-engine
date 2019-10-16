#!/bin/sh
#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Test the elapsed time for a process since running.
$0 <PID>

Examples:
$0 \$(ps a -o comm,pid | grep "json-feeder.py" | awk '{print \$2}')
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

PID=$1
while ps --pid $PID >/dev/null; do ps --pid $PID -o comm,etime; date; sleep 1; done
