#!/bin/bash

today=`date +"%Y%m%d"` 
send_status_file=/tmp/.query_send_status ;
>$send_status_file ;

rm -f `ls /tmp/.query_status* 2>/dev/null | grep -v "$today"` ;

for ids in `ps -ef |grep tradeinit | grep -v "retail" |grep -v grep |  awk '{print $NF}'`
do 
  
  files=/spare/local/logs/tradelogs/log.$today"."$ids ;

  if [ ! -f $files ] ; then 
   continue ; 
  fi 

  complete_line=`egrep "resume|getflat_due_to_" $files | grep -v "000000  getflat_" | tail -n1` ; 
  last_line=`echo $complete_line | awk '{print $2}'` ;
  queryid=`echo $complete_line | awk '{print $NF}'` ; 

  current_getflat_count=`grep "getflat_due_to_" $files | grep -v "000000  getflat_" | wc -l` ;

  if [ "resume_normal_trading" != "$last_line" ] ; then 

    status_storage_file=/tmp/.query_status.$today"."$queryid ; 

    if [ ! -f $status_storage_file ] ; then 
      echo "0" > $status_storage_file ;
    fi 

    last_count=`cat $status_storage_file` ;

    if [ $current_getflat_count -gt $last_count ] ; then 

      if [ "getflat_due_to_max_position_" == "$last_line" ] ; then 
        maxpos=`grep "0.000000 max_position_" $files | tail -1 | awk '{print $NF}'` ;
        echo $queryid "HitMaxPos :"$maxpos >> $send_status_file ;
      elif [ "getflat_due_to_max_loss_" == "$last_line" ] ; then
        maxloss=`grep "0.000000 max_loss_" $files | tail -1 | awk '{print $NF}'` >> $send_status_file ;
        echo $queryid "HitMaxLoss :"$maxloss >> $send_status_file ;
      elif [ "getflat_due_to_global_max_loss_" == "$last_line" ] ; then
        gmaxloss=`grep "0.000000 global_max_loss_" $files | tail -1 | awk '{print $NF}'` ; 
        echo $queryid "HitGlblMaxLoss : "$gmaxloss >> $send_status_file ;
      elif [ "getflat_due_to_external_getflat_" == "$last_line" ] ; then
        echo $queryid "ExternalFlat" >> $send_status_file ;
      elif [ "getflat_due_to_external_agg_getflat_" == "$last_line" ] ; then
        echo $queryid "ExternalAggFlat" >> $send_status_file ;
      elif [ "getflat_due_to_funds_rej_" == "$last_line" ] ; then
        echo $queryid "HitFundsRej" >> $send_status_file ;
      elif [ "getflat_due_to_max_pnl_" == "$last_line" ] ; then
        maxpnl=`grep "0.000000 max_pnl_" $files | tail -1 | awk '{print $NF}'`
        echo $queryid "HitMaxPnl: "$maxpnl >> $send_status_file ;
      elif [ "getflat_due_to_short_term_global_max_loss_" == "$last_line" ] ; then
        sgmaxloss=`grep "0.000000 short_term_global_max_loss_" $files | tail -1 | awk '{print $NF}'`
        echo $queryid "HitShortTermGlobal: "$sgmaxloss >> $send_status_file ;
      elif [ "getflat_due_to_max_opentrade_loss_" == "$last_line" ] ; then
        omaxloss=`grep "0.000000 max_opentrade_loss_" $files | tail -1 | awk '{print $NF}'`
        echo $queryid "HitOpenLoss: "$omaxloss >> $send_status_file ;
      elif [ "getflat_due_to_economic_times_" == "$last_line" ] ; then
        echo $queryid "FlatOnEvent" >> $send_status_file ;
      elif [ "getflat_due_to_market_data_interrupt_" == "$last_line" ] ; then
        echo $queryid "MktDataInterrupt: "`echo $complete_line | awk '{print $4}'` >> $send_status_file ;
      elif [ "getflat_due_to_close_" == "$last_line" ] ; then
        :
      else 
        :
    fi 

    #Update Count Here 
    echo $current_getflat_count > $status_storage_file ;
   
  fi

 fi

done 

cat $send_status_file ;
