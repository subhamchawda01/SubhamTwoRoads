#!/bin/bash

USAGE="$0 user@ip";

if [ $# -lt 1 ] ;
then
    echo $USAGE;
    exit;
fi

server=$1

status="nok"
ret=0
#loop will continue 500 times if ssh connection has problem
while [ "$status" != "ok" ]; do
   if [ "$ret" -eq 3 ];then
      return 1
   fi
   status=$(ssh $server echo ok 2>/dev/null </dev/null)
   if [ "$status" == "ok" ] ;then
        echo "ssh success"
   else
        echo "SSH ErrorCode: "$?
        sleep 100
   fi
   ret=$((ret+1))
done
exit 0
