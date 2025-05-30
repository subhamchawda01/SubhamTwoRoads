#!/bin/bash

[ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DATE >" ;

date=$1;

if [ "$1" == "YESTERDAY" ] ; then
  date=`date -d "1 day ago" +"%Y%m%d"` ;
fi

servers="INDB12" ;
for server in $servers;
do
  cp /run/media/root/Elements/SERVERDATA/$server/tradelogs/log.$date.* /home/dvcinfra/PNLProject/copy/$server/
done


