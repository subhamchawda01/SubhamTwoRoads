#!/bin/bash

YYYYMMDD=$(date "+%Y%m%d");                                                                                                                                                                               
YYYYMMDD=`$HOME/basetrade_install/bin/calc_prev_day $YYYYMMDD 10`;
yyyy=${YYYYMMDD:0:4}; mm=${YYYYMMDD:4:2}; dd=${YYYYMMDD:6:2};
prev_yyyy=`expr $yyyy - 1`; prev_mm=`expr $mm - 1`;

#ip_list=`cat ~/AWSScheduler/instance_cores.txt | awk '{if($2>0){ print $1; }}'`;
ip_list=`cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print $4}'`;

for ip in $ip_list; do
  ssh $ip "for year in \`seq 2010 $prev_yyyy\`; do rm -rf /media/ephemeral0/404/NAS1/data/*/*/\$year; rm -rf /media/ephemeral0/primary_404/NAS1/data/*/*/\$year; done ; for month in \`seq 1 $prev_mm\`; do if [ \$month -lt 10 ] ; then month=0\$month; fi ; rm -rf /media/ephemeral0/404/NAS1/data/*/*/$yyyy/\$month; rm -rf /media/ephemeral0/primary_404/NAS1/data/*/*/$yyyy/\$month; done ; for day in \`seq 1 $dd\`; do if [ \$day -lt 10 ] ; then day=0\$day; fi ; rm -rf /media/ephemeral0/404/NAS1/data/*/*/$yyyy/$mm/\$day; rm -rf /media/ephemeral0/primary_404/NAS1/data/*/*/$yyyy/$mm/\$day; done"; 
done
