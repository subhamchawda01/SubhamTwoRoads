#!/bin/bash

USAGE1="$0 DATE"
EXAMP1="$0 YESTERDAY"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;

if [ "$YYYYMMDD" == "YESTERDAY" ]
then 

    YYYYMMDD=`cat /tmp/YESTERDAY_DATE`

fi 

if [ "$YYYYMMDD" == "TODAY" ]
then 

    YYYYMMDD=`date +"%Y%m%d"` ;

fi 


#export NEW_GCC_LIB=/usr/local/lib
#export NEW_GCC_LIB64=/usr/local/lib64
#export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


AUDIT_LOG_GENERATE_EXEC=$HOME/infracore_install/bin/generate_newedge_audit_log 
EXCH_SYMBOL_EXEC=$HOME/infracore_install/bin/get_exchange_symbol 
AUDIT_LOG_GENERATE_SCRIPT=$HOME/LiveExec/scripts/process_cme_log_to_audit_trail.pl
SHORTCODE_FILE_LIST=/spare/local/files/AUDIT/newedge_CME_new_audit_config.cfg
TEMP_FILE_FOR_TRADES=/tmp/newedge_trades_temp.txt 
TRANSACTION_NUMBER_FILE=/spare/local/files/AUDIT/cme_transaction_number.txt #keeps track of number of transaction in the day so far
DESTINATION_SUDIT_LOG_DIR=/apps/data/MFGlobalTrades/AUDIT; #/home/diwakar/AUDIT 
SOURCE_AUDIT_DIR=/apps/data/MFGlobalTrades/AUDIT/FIX_LOGS; #$DESTINATION_SUDIT_LOG_DIR/real_data/;
ORS_TRADES_FILE=/NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_;
AUDIT_LOG_FILE=$YYYYMMDD".AUDIT.log" ;
yyyy=`echo ${YYYYMMDD:0:4}` ;
mm=`echo ${YYYYMMDD:4:2}` ;
dd=`echo ${YYYYMMDD:6:2}` ;

declare -A prod_to_trades;
market_code_="glbx";
platform_code_="prop";
client_name_="dvcapital";
extension_="audittrail.csv";
done_extension_="audittrail.done";
ecid="9648237";
num_lines=0;
common_string_='';
total_trde_size_=;
product="";
for session_ in `ls $SOURCE_AUDIT_DIR| grep $YYYYMMDD  | sed s/'\.'/' '/g | awk '{print $3} ' | sort | uniq`
do
    session_id_files_=`ls $SOURCE_AUDIT_DIR/ | grep $YYYYMMDD |grep $session_`;
    temp_audit_file_=/tmp/$YYYYMMDD.$session_;
    cat /dev/null > $temp_audit_file_".in";
    cat /dev/null > $temp_audit_file_.".out";
    for in_files_ in `ls $SOURCE_AUDIT_DIR/*" | grep $YYYYMMDD |grep $session_`;
    do
        cat $in_files_ >> $temp_audit_file_".all";
    done
#done

#for audit_session_ in $SOURCE_AUDIT_DIR/$YYYYMMDD".QGV5N2N"
#do 
    session_code_="${audit_session_##*.}";
    session_code_=$session_;
    additional_details_="";
    this_dest_dir=$DESTINATION_SUDIT_LOG_DIR"/"$yyyy"/"$mm"/"$dd ;
    mkdir -p $this_dest_dir ;
    final_out_file_name_=$YYYYMMDD"_"$ecid"_"$market_code_"_"$platform_code_"_"$session_code_"_"$client_name_"_"$additional_details_$extension_;
    cat /dev/null > $this_dest_dir/$final_out_file_name_;
    temp_file_name_="temp"
    touch $this_dest_dir/$temp_file_name_;
    cat /dev/null > $this_dest_dir/$temp_file_name_;
    for type in ".all"
    do
       file=/tmp/$YYYYMMDD.$session_$type #$audit_session_$type;
       FIX_LOG_FILE=$file;
       tnf_date=`cat $TRANSACTION_NUMBER_FILE | awk '{print $1}'`
       if [[ $tnf_date == $YYYYMMDD ]]; then
            server_tranction_number=`cat $TRANSACTION_NUMBER_FILE | awk '{print $2}'` ;
       else
            server_tranction_number=0;
       fi
       out_file_name_=$YYYYMMDD"_"$ecid"_"$market_code_"_"$platform_code_"_"$session_code_"_"$client_name_"_"$additional_details_$extension_$type;
#echo "perl $AUDIT_LOG_GENERATE_SCRIPT $YYYYMMDD $SHORTCODE_FILE_LIST $FIX_LOG_FILE $server_tranction_number 0 > $this_dest_dir/$out_file_name_";
       perl $AUDIT_LOG_GENERATE_SCRIPT $YYYYMMDD $SHORTCODE_FILE_LIST $FIX_LOG_FILE $server_tranction_number 0 > $this_dest_dir/$out_file_name_;
       common_string_=`head -n1 $this_dest_dir/$out_file_name_`;
       sed '1d' $this_dest_dir/$out_file_name_ > $this_dest_dir/$out_file_name_"temp";
       this_trade_size_=0;
       for product in `grep FILL $this_dest_dir/$out_file_name_"temp" | cut -d ',' -f20 | sort | uniq ` ; do

         this_trade_size_=`grep FILL $this_dest_dir/$out_file_name_"temp" | grep $product | cut -d ',' -f18 | awk '{sum+=$1} END {print sum}'`;
         if [[ $this_trade_size_ = *[[:digit:]]* ]]; then
            if [[ ${prod_to_trades["$product"]} = *[[:digit:]]* ]] ;then
               echo "earlier entry exists for $product --- ${prod_to_trades["$product"]}";
               ets=${prod_to_trades["$product"]};
               unset -v prod_to_trades["$product"];
               prod_to_trades+=( ["$product"]=$(($ets + $this_trade_size_)) );
            else
               echo "no earlier entry for: $product ---- ${prod_to_trades[$product]}";
               prod_to_trades+=( ["$product"]=$this_trade_size_ );
               echo ${prod_to_trades["$product"]};
            fi
            let total_trde_size_=$(($total_trde_size_ + $this_trade_size_));
        fi
      done

       cat $this_dest_dir/$out_file_name_"temp" >> $this_dest_dir/$temp_file_name_;
echo `cat $this_dest_dir/$out_file_name_ | tail -n1 `;
       new_server_transaction_number=`cat $this_dest_dir/$out_file_name_ | tail -n1 | cut -d ',' -f1 `;
       if [[ $new_server_transaction_number = *[[:digit:]]* ]] ;then
           echo "$YYYYMMDD $new_server_transaction_number" > $TRANSACTION_NUMBER_FILE;
       fi
       rm $this_dest_dir/$out_file_name_;
       rm $this_dest_dir/$out_file_name_"temp";
       rm $FIX_LOG_FILE;
    done
    sort -t"," -k2 $this_dest_dir/$temp_file_name_ > $this_dest_dir/$temp_file_name_"temp";
    num_lines=`wc -l $this_dest_dir/$temp_file_name_| awk '{print $1}'`;
    echo $common_string_ > $this_dest_dir/$final_out_file_name_;
    cat $this_dest_dir/$temp_file_name_"temp" >> $this_dest_dir/$final_out_file_name_;
    rm $this_dest_dir/$temp_file_name_;
    done_file_name_=$YYYYMMDD"_"$ecid"_"$market_code_"_"$platform_code_"_"$session_code_"_"$client_name_"_"$additional_details_$done_extension_;
    echo "RECORD_COUNT=$num_lines" > $this_dest_dir/$done_file_name_;
    rm $this_dest_dir/$temp_file_name_"temp";
    cd $this_dest_dir;
# zip $YYYYMMDD"_"$ecid"_"$market_code_"_"$platform_code_"_"$session_code_"_"$client_name_"_"$additional_details_"done.zip" $done_file_name_;
    nm=${final_out_file_name_%.csv};
	echo "Files: ".$nm;
    zip $nm".zip" $final_out_file_name_;

done;
for prod in "${!prod_to_trades[@]}"; 
do 
ors_trade_size_=`grep $prod $ORS_TRADES_FILE$YYYYMMDD".txt" | awk '{print $14}'`;
if [[ ${prod_to_trades["$prod"]} == $ors_trade_size_ ]]; then
echo "Trade sizes match..";
else
   echo "Audit and ors trade sizes do not match for product $prod. AUDIT size: ${prod_to_trades["$prod"]} and ORS trade size: $ors_trade_size_."| /bin/mail -s "NewedgeAuditLogsAlert" -r "auditlogsgenerator@ny11" "diwakar@circulumvite.com,nseall@tworoads.co.in";
fi
done
ls $this_dest_dir/;

rm $this_dest_dir/*.csv;
#rm $this_dest_dir/*."done";
#exit;
HOST='ftp.newedgegroup.com';
USER='SDMA-DVCapital';
PASSWD='G71lrKqh#';

RSA_PRIVATE_KEY=/home/dvcinfra/.ssh/id_rsa.legacy

#### SFTP #### fetch cash statements txt
#Newedge files
sftp -b - -oIdentityFile=$RSA_PRIVATE_KEY $USER@$HOST << !

cd AuditLogFiles
put $this_dest_dir/*audittrail.zip
rm ./*9QXN_dvcapital_audittrail.zip
put $this_dest_dir/*done
rm ./*9QXN_dvcapital_audittrail.done
bye
!

#ED&F
sftp -b - -oPort=2222 DV_Capital@ftpc.edfmancapital.com << !

put $this_dest_dir/*9QXN*audittrail.zip
put $this_dest_dir/*9QXN*done
bye
!

