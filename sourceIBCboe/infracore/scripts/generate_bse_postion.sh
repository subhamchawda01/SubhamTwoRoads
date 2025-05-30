#!/bin/bash

#Main 
if [ $# -lt 1 ] ; then
  echo "USAGE: <TRADE_EXPORT_POSITION_FILE>"
  echo "USAGE: <TRADE_EXPORT_POSITION_FILE> BATCH_ORDER"
  exit
fi

File_net_pos=$1
date_=`date +%Y%m%d`
curr_time=`TZ=Asia/Kolkata date +%H_%M`

out_pos="/tmp/bse_position_generated_$date_"
out_pos_="/tmp/bse_bolt_position_batch_order_${date_}.csv"


if [ $# -eq 1 ] ; then
	cat $File_net_pos  | awk -F',' 'NR > 1 {if ($7 != 0) {print "BSE_"$1,$7 * -1}}' > $out_pos
	echo
	echo "Postion generated In File $out_pos"
elif [ $2 == "BATCH_ORDER" ] ; then
	echo "Generating positions for BSE Bolt Pro";
	echo "Buy/Sell,Qty,Rev.Qty,Scrip Code,Rate,Short/Client ID,Retention Status,Client Type,Order Type,CP Code,TrgRate" > $out_pos_ 
        
	cat $File_net_pos  | awk -F',' 'NR > 1 {if ($7 != 0 && $7 < 0 ) {print "B,"  $7*(-1) "," $7*(-1) "," $2 "," $4 "," "OWN," "EOTODY," "OWN," "G,," }else if ($7 != 0 && $7 > 0 ) {print "S,"  $7 "," $7 "," $2 "," $4 "," "OWN," "EOTODY," "OWN," "G,," } } ' >> $out_pos_
  sshpass -p 'tworoads321$' scp $out_pos_ DELL@10.23.5.20:"C:\\Users\\DELL\\Desktop\\BSE_Positon_file\\bse_bolt_position_batch_order_${date_}_${curr_time}.csv"
  echo "Batch order file generated in file $out_pos_ , DELL@10.23.5.20:C:\\Users\\DELL\\Desktop\\BSE_Positon_file\\bse_bolt_position_batch_order_${date_}_${curr_time}.csv"

else
        echo "USAGE: <TRADE_EXPORT_POSITION_FILE>"
        echo "USAGE: <TRADE_EXPORT_POSITION_FILE> BATCH_ORDER"
	exit
fi


