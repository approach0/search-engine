#!/bin/bash
if [ "$1" == "-h" ]; then
cat << USAGE
Description:
Run command and respawn upon non-zero return.

Examples:
$0 ./run/searchd.out -i ~/index &
$0 kill
USAGE
exit
fi

[ $# -eq 0 ] && echo 'no command specified.' && exit

if [ "$1" == "kill" ]; then
	kill `cat respawn.pid.log`
	killall `cat respawn.cmd.log`
	exit
fi;

cmd="${1}"
cmd_name="`basename ${cmd}`"
shift
args="$*"

echo "$$" > respawn.pid.log
echo "${cmd_name}" > respawn.cmd.log

while ((1)); do
	"${cmd}" $args

	retcode=$?
	if [ $retcode -eq 0 ]; then
		break
	else
		echo "${cmd} non-zero retcode $?" >> respawn.log
	fi;

	sleep 1
done
