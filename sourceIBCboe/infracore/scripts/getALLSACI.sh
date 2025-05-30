#!/bin/bash

DIR='/spare/local/logs/tradelogs/';
STRAT_IDS='123422 123423 123424 123552'

DATE=$1;

if [ "$1" == "TODAY" ] ; then
  DATE=`date +"%Y%m%d"`;
fi

SACI_list_file='/tmp/SACI_list.'$DATE ;

true>$SACI_list_file
#echo -e "date-> $DATE \n" ;

for strat in $STRAT_IDS;
do
  echo $DIR"log."$DATE"."$strat
  zgrep "RMC_SACI" $DIR"log."$DATE"."$strat | awk '{print $6}' >> $SACI_list_file ;
done
scp $SACI_list_file dvcinfra@10.23.227.63:$SACI_list_file
