#!/bin/bash 

DATA_GENERATOR=/home/dvcinfra/LiveExec/bin/nse_convert_nse_raw_data_to_internal_struct 
epoch_date=19700101
nse_start_date=19800101

usage () {

  echo "<Usage> <NSEDataDirectory>" ; 
  echo "<Example> /NAS1/data/NSEData/CD" ; 
  exit ; 

}

generate_data () {

  for dts in `find $1 -type f  |  grep "Trades" | awk -F"_" '{print $3}' | awk -F"." '{print $1}'` 
  do

     dd=${dts:0:2} ; 
     mm=${dts:2:2} ;
     yy=${dts:4:4} ;

     dt=`echo $yy$mm$dd` ; 

     a=`date -d $dt +%s` ; 
     b=`date -d $nse_start_date +%s` ; 
     c=`date -d $epoch_date +%s` ; 

     jiffies_diff=$(($((a-b))*65535)) ; 
     timeval_diff=$((a-c)) ; 

     order_file=`find $1 -type f | grep $dts | grep "Orders"` ; 
     trade_file=`find $1 -type f | grep $dts | grep "Trades"` ;

     $DATA_GENERATOR $order_file $trade_file $jiffies_diff $dt $timeval_diff >> /home/dvcinfra/converted_data_status.txt ;

     gzip `find /home/dvcinfra/NSEDataInternalStructs -type f` ;
     rsync -avz --progress /home/dvcinfra/NSEDataInternalStructs dvcinfra@10.23.74.40:/apps/data ; 

     rm -rf /home/dvcinfra/NSEDataInternalStructs ; 

  done 

}

main () {

  if [ $# -lt 1 ] ;
  then

     usage ;

  else 
     
     generate_data $* 

  fi 

}

main $* ;

