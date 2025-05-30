#!/bin/bash

USAGE1="$0 YYYYMM [MAIL]"
EXAMP1="$0 ThisMonth [MAIL]"

if [ $# -lt 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMM=$1
SEND_MAIL="NOMAIL"

if [ $# -gt 1 ] ;
then 

    SEND_MAIL=$2 ;

fi

if [ $YYYYMM = "ThisMonth" ] 
then

    YYYYMM=`date +"%Y%m"` ;

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH



VOL_SCRIPT=$HOME/infracore_install/scripts/get_our_total_traded_volumes_for_shortcode.sh
MAIL_FILE=/tmp/mail_our_volumes.txt

#Currently Only being used for LFZ
#ListOfSymbols="LFZ";

symbol="LFZ" ;

TotalVolume=`$VOL_SCRIPT $symbol $YYYYMM "SUMMARY" | awk '{print $1}'` ;

echo "TotalVolumes For : " $symbol " For : " $YYYYMM " " $TotalVolume ; 

CurrAccName="15038200" ;
CurrAccNumber="15038200" ;

TEMP_CONF=/tmp/setaccount_temp.conf 

#200 
if [ $TotalVolume -le 2000 ] 
then 

    CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
    CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

    if [ "$CurrAcc" == "15038200" ] 
    then 

        echo "Using Account : 15038200" ;

    else 

        ssh dvcinfra@10.23.52.52 'cat /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg | egrep -v "AccountNumber|AccountName"' > $TEMP_CONF ;

        echo "AccountNumber 15038200" >> $TEMP_CONF ;
        echo "AccountName 15038200" >> $TEMP_CONF ;

        scp $TEMP_CONF dvcinfra@10.23.52.51:/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg ; 

        CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
        CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

        echo "Total LFZ Volumes this month : " $TotalVolume " Shifting Account To : " $CurrAccNumber >> $MAIL_FILE ;

    fi 

fi 

#201 
if [ $TotalVolume -gt 2000 ] && [ $TotalVolume -le 4000 ] 
then 

    CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
    CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

    if [ "$CurrAcc" == "15038201" ] 
    then 

        echo "Using Account : 15038201" ;

    else 

        ssh dvcinfra@10.23.52.52 'cat /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg | egrep -v "AccountNumber|AccountName"' > $TEMP_CONF ;

        echo "AccountNumber 15038201" >> $TEMP_CONF ;
        echo "AccountName 15038201" >> $TEMP_CONF ;

        scp $TEMP_CONF dvcinfra@10.23.52.51:/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg ; 

        CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
        CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

        echo "Total LFZ Volumes this month : " $TotalVolume " Shifting Account To : " $CurrAccNumber >> $MAIL_FILE ;

    fi 

fi 

#202 
if [ $TotalVolume -gt 4000 ] && [ $TotalVolume -le 6000 ] 
then 

    CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
    CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

    if [ "$CurrAcc" == "15038202" ] 
    then 

        echo "Using Account : 15038202" ;

    else 

        ssh dvcinfra@10.23.52.52 'cat /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg | egrep -v "AccountNumber|AccountName"' > $TEMP_CONF ;

        echo "AccountNumber 15038202" >> $TEMP_CONF ;
        echo "AccountName 15038202" >> $TEMP_CONF ;

        scp $TEMP_CONF dvcinfra@10.23.52.51:/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg ; 

        CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
        CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

        echo "Total LFZ Volumes this month : " $TotalVolume " Shifting Account To : " $CurrAccNumber >> $MAIL_FILE ;

    fi 


fi 

#203 
if [ $TotalVolume -gt 6000 ] && [ $TotalVolume -le 8000 ] 
then 

    CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
    CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

    if [ "$CurrAcc" == "15038203" ] 
    then 

        echo "Using Account : 15038203" ;

    else 

        ssh dvcinfra@10.23.52.52 'cat /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg | egrep -v "AccountNumber|AccountName"' > $TEMP_CONF ;

        echo "AccountNumber 15038203" >> $TEMP_CONF ;
        echo "AccountName 15038203" >> $TEMP_CONF ;

        scp $TEMP_CONF dvcinfra@10.23.52.51:/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg ; 

        CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
        CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

        echo "Total LFZ Volumes this month : " $TotalVolume " Shifting Account To : " $CurrAccNumber >> $MAIL_FILE ;

    fi 


fi 

#Default 200 
if [ $TotalVolume -gt 8000 ] 
then 

    CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
    CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

    if [ "$CurrAcc" == "15038200" ] 
    then 

        echo "Using Account : 15038200" ;

    else 

        ssh dvcinfra@10.23.52.52 'cat /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg | egrep -v "AccountNumber|AccountName"' > $TEMP_CONF ;

        echo "AccountNumber 15038200" >> $TEMP_CONF ;
        echo "AccountName 15038200" >> $TEMP_CONF ;

        scp $TEMP_CONF dvcinfra@10.23.52.51:/home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg ; 

        CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
        CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

        echo "Total LFZ Volumes this month : " $TotalVolume " Shifting Account To : " $CurrAccNumber >> $MAIL_FILE ;

    fi 

fi 


#for symbol in $ListOfSymbols
#do

#    $VOL_SCRIPT $symbol $YYYYMM "SUMMARY"

#done


CurrAcc=`ssh dvcinfra@10.23.52.51 'grep "AccountName" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`
CurrAccNumber=`ssh dvcinfra@10.23.52.51 'grep "AccountNumber" /home/dvcinfra/infracore_install/Configs/OrderRoutingServer/cfg/4M0/ors.cfg' | awk '{print $2}'`

if [ "$CurrAcc" != "15038200" ] && [ "$CurrAcc" != "15038201" ] && [ "$CurrAcc" != "15038202" ] && [ "$CurrAcc" != "15038203" ] 
then 

    echo " ALERT Account Setup Failed, Current Account Name : " $CurrAcc >> $MAIL_FILE ;

fi 

if [ "$CurrAccNumber" != "15038200" ] && [ "$CurrAccNumber" != "15038201" ] && [ "$CurrAccNumber" != "15038202" ] && [ "$CurrAccNumber" != "15038203" ] 
then

    echo " ALERT Account Setup Failed, Current Account Number : " $CurrAccNumber >> $MAIL_FILE ; 

fi 


if [ $SEND_MAIL = "MAIL" ]
then 
    HOSTNAME=`hostname`;
    /bin/mail -s "LFZ Rebate Account Setup" -r "nseall@tworoads.co.in" "ravi.parikh@tworoads.co.in" < $MAIL_FILE

else

    cat $MAIL_FILE

fi

rm -rf $MAIL_FILE ;
rm -rf $TEMP_CONF ;
