#!/bin/sh
#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Test the elapsed time for a process since running.
$0 <process name>

Examples:
$0 indexerd.out
USAGE
exit
fi

[ $# -ne 1 ] && echo 'bad arg.' && exit

PROCESS="$1"
while pidof "$PROCESS" >/dev/null; do ps --pid $(pidof "$PROCESS") -o command,etime; date; sleep 1; done
