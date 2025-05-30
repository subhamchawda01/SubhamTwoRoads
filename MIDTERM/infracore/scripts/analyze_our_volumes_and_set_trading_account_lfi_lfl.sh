#!/bin/bash

#Flag to decide send mail or not
SEND_MAIL="NOMAIL"

if [ $# -gt 0 ] ;
then

    SEND_MAIL=$1 ;

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

VOL_SCRIPT=$HOME/infracore_install/scripts/get_our_total_traded_volumes_for_shortcode.sh
MAIL_FILE=/tmp/mail_our_lfi_lfl_volumes.txt
TEMP_CONF=/tmp/setaccount_temp_lfi_lfl.conf

total_volume=0
current_month=`date +"%Y%m"`
#Nov 2014
first_month=201411
host=10.23.52.53
ors_config_file=/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/MSICE2/ors.cfg

#Iterate over all months since Nov 2014
while [ $first_month -le $current_month ]
do

  echo "Calculating vol for month: " $first_month
  #LFI volume for this month
  temp_month_vol=`$VOL_SCRIPT LFI $first_month SUMMARY`
  total_volume=$(($total_volume+$temp_month_vol))
  
  #LFL volume for this month
  temp_month_vol=`$VOL_SCRIPT LFL $first_month SUMMARY`
  total_volume=$(($total_volume+$temp_month_vol))
  
  first_month=`date +"%Y%m" --date=$first_month"15 next month"`

done

account_increment=$(($total_volume/70000))

echo "Total volume for LFI-LFL since Nov 2014: " $total_volume "To increment: " $account_increment

if [ $account_increment -gt 3 ]
then
  account_increment=0
fi

#If we need to switch accounts ( volume > 70k )
if [ $account_increment -ge 1 ]
then

  account_increment=$(($account_increment+15038205))
  
  #Fetching present account info
  echo "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no dvcinfra@$host 'grep "ClearingAccount" $ors_config_file' | awk '{print \$2}'"
  CurrAcc=`ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no dvcinfra@$host grep "ClearingAccount" $ors_config_file | awk '{print \$2}'`
  echo "CurrAcc: " $CurrAcc "Expected: " $account_increment  
  #No need to update
  if [ "$CurrAcc" == "$account_increment" ]
  then

    echo "Already Using Account : $account_increment" ;

  else

    scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no dvcinfra@$host:$ors_config_file $TEMP_CONF"_tmp"
    grep -v "ClearingAccount" $TEMP_CONF"_tmp" > $TEMP_CONF ;

    echo "ClearingAccount $account_increment" >> $TEMP_CONF ;

    scp $TEMP_CONF dvcinfra@$host:$ors_config_file ;

    OldAcc=$CurrAcc
    CurrAcc=`ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no dvcinfra@$host grep "ClearingAccount" $ors_config_file | awk '{print \$2}'`

    echo "Total LFI-LFL Volumes till now, since 20141101 : " $total_volume " Shifting Account To : " $account_increment ", Old Acc No. " $OldAcc", New Acc No." $CurrAcc >> $MAIL_FILE ;

  fi
fi

#Send mail if opted for
if [[ $SEND_MAIL = "MAIL" && -e $MAIL_FILE ]]
then
    HOSTNAME=`hostname`;
    echo "Sending mail"
    /bin/mail -s "LFI-LFL Rebate Account Setup" -r "chandan.kumar@tworoads.co.in" "nseall@tworoads.co.in" < $MAIL_FILE

elif [ -e $MAIL_FILE ]
then

    cat $MAIL_FILE

fi

rm -rf $MAIL_FILE ;
rm -rf $TEMP_CONF ;
rm -rf $TEMP_CONF"_tmp" ;
