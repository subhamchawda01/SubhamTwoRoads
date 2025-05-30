#!/bin/bash

# USAGE:
# 
# tars & gunzips the logs saved to the directories in '/spare/local/MDSlogs/'
# Sends them over to the specified exchange.

USAGE="$0    EXCHANGE    CURRENT_LOCATION    YYYYMMDD [exch_timing]";
USAGE_DESC="Syncs logs for exchange 'EXCHANGE' dated 'YYYYMMDD' to 'CURRENT_LOCATION' server.";

if [ $# -lt 3 ] ;
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

USER="dvcinfra"

EXCHANGE=$1;
CURRENT_LOCATION=$2;
YYYYMMDD=$3;
exch_timing=$4;

DEST_SERVER="10.23.74.41"
MDSLOG_DIR="/spare/local/MDSlogs";

#helps track datacopy on zabbix
update_datacopy_status(){
  status=$1 ;
  is_exit=`echo $1 | awk -F"_" '{print $1}'` ;
  ssh $USER@$DEST_SERVER "echo '$status' > $datacopy_tracker_file" ;

  if [ "$is_exit" = "FAILED" ] ; then 
    exit;
  fi 
}

#Dump MDS files (from CombinedShmWriter) before initiating datacopy
/home/dvcinfra/LiveExec/bin/combined_user_msg --dump_mds_files --only_ors_files 0

COPY_OVER_NON_COMBINED_SCRIPT=$HOME/LiveExec/scripts/copy_over_noncombined_logged_data.sh

#`$COPY_OVER_NON_COMBINED_SCRIPT /spare/local/MDSlogs/NonCombined/$EXCHANGE /spare/local/MDSlogs/GENERIC /spare/local/MDSlogs/$EXCHANGE $YYYYMMDD` ;

DO_NOT_COPY_LOGS_FILE=/spare/local/MDSlogs/"DONT_COPY_"$EXCHANGE

if [ $YYYYMMDD = "TODAY" ] ;
then

    #if the DO_NOT_COPY file was modified in last 20 hrs don't copy data
    if [ `find $DO_NOT_COPY_LOGS_FILE -mmin -1200 -print 2>/dev/null | wc -l` == 1 ]
    then
        echo "Data Logged For "$EXCHANGE" At "$CURRENT_LOCATION" Is Not To Be Copied Over"
        echo "Remove File "$DO_NOT_COPY_LOGS_FILE" To Copy Forcefully"
        update_datacopy_status "FAILED_DONOTCOPY" ;
        exit;
    fi

    YYYYMMDD=$(date "+%Y%m%d")
fi

#Create/Empty Zabbix Tracking File 
datacopy_tracker_file=~/important/DATACOPY_$EXCHANGE"_"$CURRENT_LOCATION"_"$YYYYMMDD;
>$datacopy_tracker_file ;

#if [ "$EXCHANGE" = "MinuteBar" ] ; then 
  #echo "MINUTEBAR GENERATOR CALLED FROM `hostname`" | /bin/mail -s "MINUTEBAR-GENERATOR" -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in" ;
  #touch /tmp/i_have_generated_minute_bar_data ;
#fi

#if [ "$CURRENT_LOCATION" = "COMMON" ] ; then 
  #echo "MINUTEBAR GENERATOR CALLED FROM `hostname`" | /bin/mail -s "MINUTEBAR-GENERATOR" -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in" ;
  #touch /tmp/i_have_generated_minute_bar_data ;
#fi 

update_datacopy_status "INITIATED" ; 

DATA_CONVERTER_EXEC=$HOME/LiveExec/bin/generic_data_converter

#HACK FOR OTHER EXCHANGES DATA AT NSE FASTER CONVERSION
exch=$EXCHANGE ; 
if [ "$exch" = "EOBIPriceFeed" ] ; then 
  exch="EOBI" ;
fi 

if [ "$exch" = "OSEPF" ] ; then 
  exch="OSE" ;
fi 

products_list=`egrep "NSE|OSE" /spare/local/files/filtered_security_for_asx.txt | grep "$exch" | sed 's/SGX_//g' | awk -F"_" '{print $1}' | tr ' ' '|'` ;

for files in `ls /spare/local/MDSlogs/GENERIC/*$YYYYMMDD* | grep -v "NSE_" | egrep "$products_list"`
do

   `$DATA_CONVERTER_EXEC $EXCHANGE $YYYYMMDD $files /spare/local/MDSlogs /spare/local/ORSBCAST $EXCHANGE`

done


echo $0" : Moving into " $MDSLOG_DIR"/"$EXCHANGE

cd $MDSLOG_DIR/$EXCHANGE

#Convert to old struct
# We don't need this as Data converter already takes care of this
if [ "$EXCHANGE" = "ICE_FOD" ] || [ "$EXCHANGE" = "ICE" ] || [ "$EXCHANGE" = "AFLASH" ]; then
  echo "Structs incompatible for $EXCHANGE. Converting to old format"
  for data_file in `ls *$YYYYMMDD`;
  do
    filename=`basename $data_file`
    $HOME/LiveExec/bin/live_struct_to_sim_struct_converter $EXCHANGE $data_file $MDSLOG_DIR/CONVERTED_$EXCHANGE/$filename
  done
#  #change dir to the converted path
  cd $MDSLOG_DIR/CONVERTED_$EXCHANGE
fi

update_datacopy_status "CONVERTED" ;

# Archiving the logs.
ARCH_NAME=$EXCHANGE$CURRENT_LOCATION$YYYYMMDD'.tar'

echo $0" : Archiving logs to "$ARCH_NAME
#gzip -f *$YYYYMMDD
for data_file in `ls *$YYYYMMDD`;
do
  gzip -c $data_file > $data_file.gz
done

tar -cvf $ARCH_NAME *$YYYYMMDD.gz

if [ -f $ARCH_NAME ] && [ -s $ARCH_NAME ] ; then 
  update_datacopy_status "ARCHIVED" ; 
else 
  update_datacopy_status "FAILED_ARCHIVED" ;
fi 

ERRORCODE=`echo $?` ;
# Store the return code for error checking.
echo $ERRORCODE > /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : Sending logs to "$DEST_SERVER

# Must add RSA public key for this to work without prompting for a password.

rsync -avz $ARCH_NAME $USER@$DEST_SERVER:/apps/data/

###scp $ARCH_NAME $USER@$DEST_SERVER:/NAS1/data/

ERRORCODE=`echo $?` ;

# Store the return code for error checking.
echo $ERRORCODE >> /tmp/$EXCHANGE"_"$YYYYMMDD

if [ $ERRORCODE -eq 0 ] ; then 
  update_datacopy_status "SYNCED_NAS" ; 
else
  update_datacopy_status "FAILED_NASSYNC" ;
fi 

echo $0" : Connecting to "$DEST_SERVER
# Must add RSA public key for this to work without prompting for a password.
# Run local script on remote server.
# note the mds_unpack path...
ssh $USER@$DEST_SERVER 'bash -s' < ~/LiveExec/scripts/mds_log_unpack.sh $EXCHANGE $CURRENT_LOCATION $YYYYMMDD $exch_timing

ERRORCODE=`echo $?` ;

if [ $ERRORCODE -eq 0 ] ; then 
  update_datacopy_status "SUCCESS" ;
else 
  update_datacopy_status "FAILED_UNPACK" ;
fi 

# Store the return code for error checking.
echo $ERRORCODE >> /tmp/$EXCHANGE"_"$YYYYMMDD

echo $0" : erasing "$ARCH_NAME
rm $ARCH_NAME

# Find if there was an error
ERRORCODE=$(grep -v "0" /tmp/$EXCHANGE"_"$YYYYMMDD | wc -l)

if [ $ERRORCODE != "0" ] ;
then
    echo $0" : ERROR => Copy FAILED."
    exit;
fi

echo $0" : SUCCESS => Copy completed."

if [ "$EXCHANGE" == "NSE" ] ; then 

  echo "/home/rgarg/Data_Generator/auto_daily_data_generator.sh $YYYYMMDD >> /spare/local/logs/nse_minbar_data_log.txt" >> /spare/local/logs/nse_minbar_data_log.txt
  while ! ssh dvctrader@10.0.1.15 "/home/rgarg/Data_Generator/auto_daily_data_generator.sh $YYYYMMDD >> /spare/local/logs/nse_minbar_data_log.txt";
  do
    #try again after sometime
    echo "Trying again after 2 min" >> /spare/local/logs/nse_minbar_data_log.txt
    sleep 120
  done

fi 

# Only delete this file if copy succeeded.
rm /tmp/$EXCHANGE"_"$YYYYMMDD
