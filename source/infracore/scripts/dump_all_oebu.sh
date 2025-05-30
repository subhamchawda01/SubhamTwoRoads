#!/bin/bash

T_VOL_FILE=$HOME/trades/ALL_MKT_VOLUMES_T;
VOL_FILE=$HOME/trades/ALL_MKT_VOLUMES;

function GetMarketVolumes
{

 >$T_VOL_FILE;

 CMD="tail -n 1000 /home/dvcinfra/oebu_out" ;
 SERVERS=(10.23.74.51 10.23.74.52 10.23.74.53 172.18.244.107 10.134.210.184 10.23.23.12 10.23.23.13 10.23.23.15 10.152.224.146 );
 for server in ${SERVERS[*]} ;
 do
   ssh -q $server "$CMD" >> $T_VOL_FILE ;
 done
}


while [ true ]
do

GetMarketVolumes;

cp $T_VOL_FILE $VOL_FILE ;

sleep 30;

done
