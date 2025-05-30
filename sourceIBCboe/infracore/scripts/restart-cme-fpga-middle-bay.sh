#!/bin/bash

DEBUG=0

LOG=$HOME/scripts/log/script_${0##*/}_$(date +%Y.%m.%d-%H.%M.%S-%N).log
#LOG=$HOME/reload/reload.log
ERROR=$HOME/scripts/log/error.log

exec 2> $ERROR
# exec 2>( while read line; do echo "$(date): ${line}"; done >> $ERROR)
exec 1> >(tee -ai $LOG)
# exec 1>( while read line; do echo "$(date): ${line}"; done ) >(tee -ai $LOG)

if [ $DEBUG = 0 ] ; then
    exec 4> >(xargs -i echo -e "[ debug ] {}")
else
    exec 4> /dev/null
fi

# test
# echo " debug sth " >&4
# echo " log sth normal "
# type -f this_is_error
# echo " errot sth ..." >&2
# echo " finish ..." >&2>&4

# task
# sudo /etc/init.d/ivan-daemon restart
appliance=$(head -n 1 /srv/ivan/serial-number.txt)
echo '--------------------------------------------------'
echo 'Appliance:         '$appliance
date "+Command started:   %c"
YYYYMMDD=`date +%Y%m%d-%H-%M-%S`
new_conf="cmemdp3-prod-"$YYYYMMDD

echo "Creating new configuration $new_conf"
echo -e "create-configuration $new_conf middle-bay cme_mdp3_tp_BBF_TP_2.2_23977_release_v2_1_0_n1_530 cmemdp3-prod\nexit\n" | nc -w 120 localhost 7777
echo "created conf"
sleep 5
echo "Stopping middle bay"
echo -e "stop-bay middle-bay\nexit\n" | nc -w 120 localhost 7777
echo "Stopped middle bay"
sleep 5
echo "Updating bay configuration"
echo -e "set-bay-configuration middle-bay $new_conf\nexit\n" | nc -w 120 localhost 7777
echo "Updated config"
sleep 5
echo "Starting middle-bay"
echo -e 'start-bay middle-bay\nexit\n' | nc -w 120 localhost 7777
echo "Started middle bay"
#sleep 5
echo
date "+Command completed: %c"
echo '--------------------------------------------------'
# close descriptor 4
exec 4>&-

