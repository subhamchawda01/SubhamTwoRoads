#!/bin/bash

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


YYYYMM=`date +"%Y%m"` ;

MAIL_FILE=/tmp/mail_LFI_volumes.txt
rm -rf $MAIL_FILE

prod=LFI
ors_config=/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/GA4/ors.cfg
#ors_config=/home/dvcinfra/ors.cfg


TotalVolume=$(~/infracore_install/scripts/get_our_total_traded_volumes_for_shortcode.sh $prod $YYYYMM SUMMARY | awk '{print $1}' )
if [ $TotalVolume -gt 70000 ]
then
    echo "SUBJECT: LFI Current Account is shifted ( VolThisMonth: $TotalVolume )"; 
    HOSTNAME=`hostname`;
    echo "TO: nseall@tworoads.co.in"
    echo "FROM: dvcinfra@$HOSTNAME"
#    echo "TO: rahul@tworoads.co.in"
    echo ""
    echo " >>>> Old Setup: " 
    ssh dvcinfra@10.23.52.53 "grep Account $ors_config"
    echo ""
    ssh dvcinfra@10.23.52.53 "
perl -pi -e 's{(AccountNumber 1503820)([0-3])}{\$1 . (\$2+1)%4}e' $ors_config;
perl -pi -e 's{(AccountName 1503820)([0-3])}{\$1 . (\$2+1)%4}e' $ors_config
"
    echo " >>>> Current Setup: "
    ssh dvcinfra@10.23.52.53 "grep Account $ors_config"
fi > $MAIL_FILE

if [ -s "$MAIL_FILE" ]
then
    # /bin/mail -s "LFI Rebate Account Setup" -r "nseall@tworoads.co.in" "rahul@tworoads.co.in" < $MAIL_FILE
    /usr/sbin/sendmail -t < $MAIL_FILE
 
fi
