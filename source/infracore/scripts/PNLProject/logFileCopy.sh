#!/bin/bash

[ $# -gt 0 ] || print_msg_and_exit "Usage : < script > < DATE >" ;

date=$1;

if [ "$1" == "YESTERDAY" ] ; then
  date=`date -d "1 day ago" +"%Y%m%d"` ;
fi

servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20" ;
for server in $servers;
do
  scp root@10.23.5.26:/run/media/root/Elements/SERVERDATA/$server/tradelogs/log.$date.* /home/hardik/PNLProject/copy/$server/
done


