#!/bin/bash

USAGE1="$0 YYYY "
EXAMP1="$0 2013"

if [ $# -lt 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYY=$1
XML_PARSER_EXEC=$HOME/LiveExec/scripts/parse_eti_report.py
PREV_DAY_=$HOME/LiveExec/bin/calc_prev_week_day
export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH 

export GCC_4_8_ROOT=/apps/gcc_versions/gcc-4_8_install
if [ -d $GCC_4_8_ROOT ] ; then
   export PATH=$GCC_4_8_ROOT/bin:$PATH;
   export LD_LIBRARY_PATH=$GCC_4_8_ROOT/lib64:$LD_LIBRARY_PATH ;
fi


eti_prod_session_list=(

  NTAPROD1 
  NTAPROD2
  NTAPROD3 

)

eurex_servers_list=(

  10.23.102.51 
  10.23.102.52
  10.23.102.53 
  10.23.102.54 

)

trading_products_list=(

  FGBL
  FGBM
  FGBS
  FESX
  FOAT
  FBTS
  FBTP
  FDAX

)


YYYYMMDD=`cat /tmp/YESTERDAY_DATE`;
files_to_look_for=log.$YYYY????".gz" ;
EXCHANGE_FILE_DIR="/NAS1/logs/ExchangeFiles/EUREX";
CALCULATE=$HOME/infracore/scripts/sumcalc.pl 
TEMP_SUMMARY_FILE=/tmp/eti_msg_count_summary.txt 
TEMP_STATS_FILE=/tmp/eti_msg_count_stats.txt
TEMP_MAIL_FILE=/tmp/eti_msg_summary_mail.txt
TEMP_ZIP_FILE=/tmp/;
TEMP_FILE=/tmp/$YYYYMMDD"069";
TEMP_PROD_INFO_FILE=/tmp/product_to_info_file.txt
>$TEMP_SUMMARY_FILE ;
>$TEMP_STATS_FILE ;
>$TEMP_MAIL_FILE ;

send_mail_="true";
for session in "${eti_prod_session_list[@]}"
do

   for server in "${eurex_servers_list[@]}"
   do 

      ssh $server 'zgrep "Msg Count" /spare/local/ORSlogs/EUREX/'$session'/'$files_to_look_for' 2>/dev/null' | grep "Msg Count" >> $TEMP_SUMMARY_FILE ;

   done 

done 

TOTAL_TRADED_DAYS=`cat $TEMP_SUMMARY_FILE | awk -F"/log" '{print $2}' | tr '.' ' '  | awk '{print $1}' | sort | uniq -c | wc -l` ;
TOTAL_MSG_COUNT=`cat $TEMP_SUMMARY_FILE | tr '<' ' '  | awk '{print $10+$15}' | $CALCULATE` ;
AVG_MSG_COUNT_STR=`echo $TOTAL_MSG_COUNT " " $TOTAL_TRADED_DAYS` ;
AVG_MSG_COUNT=`echo $AVG_MSG_COUNT_STR | awk '{print $1/$2}'` ;

echo "===================== ETI Message Count Summary ======================" >> $TEMP_MAIL_FILE ;
echo >> $TEMP_MAIL_FILE ; 


TEMP_PROD_SPECIFIC_FILE=/tmp/product_specific_summary.txt 


declare -A prod_to_trades;
declare -A prod_to_messages_;
declare -A days_;
day_=0;
total_messsage_=0;
for date in `ls $EXCHANGE_FILE_DIR"/" | grep $YYYY `;
do 
  > $TEMP_PROD_INFO_FILE;
  REP_69_NAME_=90RPTCB069DVCNJ$date.XML;
  REPORT69FILE=$EXCHANGE_FILE_DIR"/"$date"/"$REP_69_NAME_;
  if [ -e $REPORT69FILE ]; then 
      python $XML_PARSER_EXEC $REPORT69FILE | awk '{print $1, $2, $4}' > $TEMP_PROD_INFO_FILE;
      day_=$(( $day_ + 1 ))
  elif [ -e $REPORT69FILE".ZIP" ]; then
      cp $REPORT69FILE".ZIP" $TEMP_ZIP_FILE
      unzip -o $TEMP_ZIP_FILE/$REP_69_NAME_".ZIP"
      python $XML_PARSER_EXEC $REP_69_NAME_ | awk '{print $1,  $2,  $4}' > $TEMP_PROD_INFO_FILE;
      rm $REP_69_NAME_; 
      day_=$(( $day_ + 1 ))
  fi
 for product in `cat $TEMP_PROD_INFO_FILE | awk '{print $1}'`;  
 do  
    this_trade_size_=`cat $TEMP_PROD_INFO_FILE| grep $product | awk '{print $3}'`;
    ets=${prod_to_trades["$product"]}; 
    unset -v prod_to_trades["$product"];
    prod_to_trades+=( ["$product"]=$(($ets + $this_trade_size_)) );
#    echo $product" "$this_trade_size_" "${prod_to_trades["$product"]}; 
    ets=${days_["$product"]}; 
    unset -v days_["$product"];
    days_+=( ["$product"]=$(($ets + 1)) );

 done
 for product in `cat $TEMP_PROD_INFO_FILE| awk '{print $1}'`;  
 do  
    this_message_size_=`cat $TEMP_PROD_INFO_FILE | grep $product | awk '{print $2}'`;
    ets=${prod_to_messages_["$product"]}; 
    unset -v prod_to_messages_["$product"];
    prod_to_messages_+=( ["$product"]=$(($ets + $this_message_size_)) );
    total_messsage_=$(( $total_messsage_ + $this_message_size_ ));
 done
done


day1_=0;
for prod in "${!prod_to_trades[@]}";
do
  if [ $day1_ -lt  " ${days_["$prod"]}" ];  then
	day1_="${days_["$prod"]}" ;
  fi

echo $prod" ==> Days -> " ${days_["$prod"]}"   Total Messages ->    "${prod_to_messages_["$prod"]}"     Total trades ->   "${prod_to_trades["$prod"]} >> $TEMP_MAIL_FILE;
done

echo "==========================================================================" >> $TEMP_MAIL_FILE ;

AVG_MSG_COUNT_STR=`echo $total_messsage_ " " $day_` ;
AVG_MSG_COUNT=`echo $AVG_MSG_COUNT_STR | awk '{print $1/$2}'` ;

echo "" >> $TEMP_MAIL_FILE ;
echo "========================== Average  Stats ================================" >> $TEMP_MAIL_FILE ;
  echo  "Days -> "$day_ " Total Messages -> " $total_messsage_ " Average -> " $AVG_MSG_COUNT >> $TEMP_MAIL_FILE;

AVG_MSG_COUNT_STR=`echo $total_messsage_ " " $day1_` ;
AVG_MSG_COUNT=`echo $AVG_MSG_COUNT_STR | awk '{print $1/$2}'` ;

echo "" >> $TEMP_MAIL_FILE ;
  echo  "Days -> "$day1_ " Total Messages -> " $total_messsage_ " Average -> " $AVG_MSG_COUNT >> $TEMP_MAIL_FILE;

echo "" >> $TEMP_MAIL_FILE ;
echo "====================== Last Day's Stats ==================================" >> $TEMP_MAIL_FILE ;

TEMP_MAIL_FILE_2="/tmp/mail_file_"
last_updated_="";
>$TEMP_MAIL_FILE_2
last_updated_date_=`cat /tmp/LAST_ETI_MESSAGE_UPDATED_DATE`;
last_updated_date_=$(($last_updated_date_+1));
last_updated_date_=$(( $last_updated_date_ < $YYYYMMDD ? $last_updated_date_:$YYYYMMDD));
send_mail_=""; 
while [  $last_updated_date_ -le $YYYYMMDD ];
do   
REP_69_NAME_=90RPTCB069DVCNJ$YYYYMMDD.XML;
REPORT69FILE=$EXCHANGE_FILE_DIR"/"$YYYYMMDD"/"$REP_69_NAME_;
echo $REPORT69FILE;
if [ -e $REPORT69FILE ]; then 
    echo "On date: $YYYYMMDD " >> $TEMP_MAIL_FILE
    python $XML_PARSER_EXEC $REPORT69FILE PrintSum| awk '{print " Product: ", $1, " Total Messages: ", $2, " Traded Volume ", $4}'>> $TEMP_MAIL_FILE;
    if [ "X$last_updated_" == "X" ];
    then 
         echo "$YYYYMMDD" > /tmp/LAST_ETI_MESSAGE_UPDATED_DATE;
         last_updated_="true";
    fi 
    send_mail_="true";
elif [ -e $REPORT69FILE".ZIP" ]; then
    echo "On date: $YYYYMMDD " >> $TEMP_MAIL_FILE
    cp $REPORT69FILE".ZIP" $TEMP_ZIP_FILE
    unzip -o $TEMP_ZIP_FILE/$REP_69_NAME_".ZIP"
    python $XML_PARSER_EXEC $REP_69_NAME_ PrintSum | awk '{print " Product: ", $1, " Total Messages: ", $2, " Traded Volume ", $4}'>> $TEMP_MAIL_FILE;
    if [ "X$last_updated_" == "X" ];
    then 
         echo "$YYYYMMDD" > /tmp/LAST_ETI_MESSAGE_UPDATED_DATE;
         last_updated_="true";
    fi 
    rm $REP_69_NAME_; 
    send_mail_="true";
else
    echo " Report CB069 Not available for date $YYYYMMDD" >> $TEMP_MAIL_FILE_2;
fi

YYYYMMDD=`$PREV_DAY_ $YYYYMMDD`;
done


echo "==========================================================================" >> $TEMP_MAIL_FILE ;
if [ "X$send_mail_" != "X" ];
then
#/bin/mail -s "ETIMessageCountSummary" -r "etimsgcounter@ny11" "diwakar@tworoads.co.in" < $TEMP_MAIL_FILE ; 
/bin/mail -s "ETIMessageCountSummary" -r "etimsgcounter@ny11" "nseall@tworoads.co.in" < $TEMP_MAIL_FILE ; 
fi

if  [ "X`cat $TEMP_MAIL_FILE_2`" != "X" ]
then
/bin/mail -s " EUREX Reports" -r "nseall@tworoads.co.in" "diwakar@circulumvite.com,kputta@circulumvite.com,nseall@tworoads.co.in" < $TEMP_MAIL_FILE_2;
fi

rm -rf $TEMP_PROD_SPECIFIC_FILE ;
rm -rf $TEMP_MAIL_FILE ; 
rm -rf $TEMP_SUMMARY_FILE ;
rm -rf $TEMP_STATS_FILE ;
