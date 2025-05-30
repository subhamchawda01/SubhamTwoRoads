#!/bin/bash

FIND_INST_SCRIPT="/home/dvctrader/controller_scripts/find_instances.sh"
INST_FILE="/mnt/sdf/JOBS/all_instances.txt"
INST_FILE_BK=$INST_FILE".bk"

CORE_FILE="/mnt/sdf/JOBS/instance_cores.txt"
CORE_FILE_BK=$CORE_FILE".bk"

  $FIND_INST_SCRIPT  > $INST_FILE_BK
  md5_1=`md5sum $INST_FILE | awk '{print $1}'`
  md5_2=`md5sum $INST_FILE_BK | awk '{print $1}'`

  if [ "$md5_1" != "$md5_2" ] ; then
    for ip in `cat $INST_FILE_BK |grep running |  awk '{print $4}'`; do echo $ip" 32"; done >  $CORE_FILE_BK
    mv $CORE_FILE_BK $CORE_FILE
    mv $INST_FILE_BK $INST_FILE
  fi

