#!/bin/bash
USAGE="Usage: $0 YYYYMMDD \n\t ";

if [ $# -ne 1 ] ;
then
    echo -e $USAGE;
    exit;
fi

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

TOKDATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;

GENERAL_NEWEDGE_COMMISSION_SCRIPT=$HOME/infracore/recon/get_general_newedge_commissions.pl;
EDF_COMMISSION_SCRIPT=$HOME/infracore/recon/get_edf_commissions.pl;
CFE_COMMISSION_SCRIPT=$HOME/infracore/recon/get_cfe_newedge_commissions.pl;
OSE_COMMISSION_SCRIPT=$HOME/infracore/recon/get_ose_newedge_commissions.pl;
HK_COMMISSION_SCRIPT=$HOME/infracore/recon/get_hk_newedge_commissions.pl;
BCS_COMMISSION_SCRIPT=$HOME/infracore/recon/get_general_bcs_commissions.pl;
PLURAL_COMMISSION_SCRIPT=$HOME/infracore/recon/get_general_plural_commissions.pl;
ABN_COMMISSION_SCRIPT=$HOME/infracore/recon/get_abn_commissions.pl;

ORS_COMMISSION_SCRIPT=$HOME/infracore/recon/calc_ors_commissions.sh;
PNL_SCRIPT="$HOME/infracore/recon/see_ors_pnl.pl";
COMPARE_COMMISSION_SCRIPT=$HOME/infracore/recon/compare_broker_commissions.py;

FETCH_NEWEDGE_SCRIPT=$HOME/infracore/recon/fetch_newedge_files.sh;
FETCH_BCS_SCRIPT=$HOME/infracore/recon/fetch_bcs_files.sh;
FETCH_PLURAL_SCRIPT=$HOME/infracore/recon/fetch_plural_files.sh;
FETCH_EDF_SCRIPT=$HOME/infracore/recon/fetch_edf_files.sh;
FETCH_ABN_SCRIPT=$HOME/infracore/recon/fetch_abnamro_files.sh;

COMMISSION_DATA_DIR="/apps/data/commission_recon"
COMMISSIONS_ERR_DIR="$HOME/recon";
mkdir -p $COMMISSION_DATA_DIR;
mkdir -p $COMMISSIONS_ERR_DIR

NEWEDGE_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/newedge_commissions_$YYYYMMDD";
BACKUP_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/backup_commissions"
BCS_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/bcs_commissions_$YYYYMMDD";
PLURAL_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/plural_commissions_$YYYYMMDD";
EDF_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/edf_commissions_$YYYYMMDD";
ABN_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/abn_commissions_$YYYYMMDD";

ORS_COMMISSIONS_FILE=$COMMISSION_DATA_DIR"/ors_commissions_$YYYYMMDD";
ORS_COMMISSIONS_TOKDATE_FILE=$COMMISSION_DATA_DIR"/ors_commissions_$TOKDATE";
COMMISSIONS_ERR_FILE=$COMMISSIONS_ERR_DIR"/commission_err_log";
COMMISSIONS_RES_FILE=$COMMISSION_DATA_DIR"/result_commissions_$YYYYMMDD";

#asx recon files
ASX_ORS_TRADE_FILE=$COMMISSIONS_ERR_DIR"/asx_ors_trade_"$YYYYMMDD;
EACH_FILE=$COMMISSIONS_ERR_DIR"/asx_temp_file";

>$NEWEDGE_COMMISSIONS_FILE
>$BCS_COMMISSIONS_FILE
>$PLURAL_COMMISSIONS_FILE
>$EDF_COMMISSIONS_FILE
>$ABN_COMMISSIONS_FILE
>$ORS_COMMISSIONS_FILE
>$COMMISSIONS_RES_FILE
>$BACKUP_COMMISSIONS_FILE
>$COMMISSIONS_ERR_FILE

echo "`date` $YYYYMMDD" >>$COMMISSIONS_ERR_FILE

#fetch broker files if not available

EDF_RAW_FILE="/apps/data/EDFTrades/EDFFiles/PFDFST4_$YYYYMMDD.CSV";
if [ ! -f $EDF_RAW_FILE ] ; then
  	echo "fetching edf files for $YYYYMMDD" >>$COMMISSIONS_ERR_FILE
	$FETCH_EDF_SCRIPT $YYYYMMDD >>$COMMISSIONS_ERR_FILE 2>>$COMMISSIONS_ERR_FILE
fi
if [ ! -f $EDF_RAW_FILE ] ; then
   echo "edf files for $YYYYMMDD not fetched." >>$COMMISSIONS_RES_FILE
fi

ABN_RAW_FILE="/apps/data/AbnamroTrades/AbnamroFiles/$YYYYMMDD_abnamro_trx.csv";
if [ ! -f $ABN_RAW_FILE ] ; then
  	echo "fetching abnamro files for $YYYYMMDD" >>$COMMISSIONS_ERR_FILE
	$FETCH_ABN_SCRIPT $YYYYMMDD >>$COMMISSIONS_ERR_FILE 2>>$COMMISSIONS_ERR_FILE
fi
if [ ! -f $ABN_RAW_FILE ] ; then
   echo "abnamro files for $YYYYMMDD not fetched." >>$COMMISSIONS_RES_FILE
fi

PLURAL_RAW_FILE="/apps/data/PluralTrades/PluralFiles/InvoiceDetailed_$YYYYMMDD.csv";
if [ ! -f $PLURAL_RAW_FILE ] ; then
  	echo "fetching plural files for $YYYYMMDD" >>$COMMISSIONS_ERR_FILE
	$FETCH_PLURAL_SCRIPT $YYYYMMDD >>$COMMISSIONS_ERR_FILE 2>>$COMMISSIONS_ERR_FILE
fi
if [ ! -f $PLURAL_RAW_FILE ] ; then
   echo "plural files for $YYYYMMDD not fetched." >>$COMMISSIONS_RES_FILE
fi

BCS_RAW_FILE="/apps/data/BCSTrades/BCSFiles/"$YYYYMMDD"_trades";
if [ ! -f $BCS_RAW_FILE ] ; then
  echo "fetching bcs files for $YYYYMMDD" >>$COMMISSIONS_ERR_FILE
  $FETCH_BCS_SCRIPT $YYYYMMDD >>$COMMISSIONS_ERR_FILE 2>>$COMMISSIONS_ERR_FILE
fi
if [ ! -f $BCS_RAW_FILE ] ; then
   echo "bcs files for $YYYYMMDD not fetched." >>$COMMISSIONS_RES_FILE
fi

NEWEDGE_RAW_FILE="/apps/data/MFGlobalTrades/MFGFiles/GMIST4_$YYYYMMDD.csv"
if [ ! -f $NEWEDGE_RAW_FILE ] ; then
   echo "fetching newedge files for $YYYYMMDD" >>$COMMISSIONS_ERR_FILE
        $FETCH_NEWEDGE_SCRIPT $YYYYMMDD >>$COMMISSIONS_ERR_FILE 2>>$COMMISSIONS_ERR_FILE
fi
if [ ! -f $NEWEDGE_RAW_FILE ] ; then
   echo "newedge files for $YYYYMMDD not fetched." >> $COMMISSIONS_RES_FILE
fi

NEWEDGE_RAW_FILE="/apps/data/MFGlobalTrades/MFGFiles/GMIST4_$TOKDATE.csv"
if [ ! -f $NEWEDGE_RAW_FILE ] ; then
        echo "fetching newedge files for $TOKDATE" >>$COMMISSIONS_ERR_FILE
        $FETCH_NEWEDGE_SCRIPT $TOKDATE >>$COMMISSIONS_ERR_FILE 2>>$COMMISSIONS_ERR_FILE
fi
if [ ! -f $NEWEDGE_RAW_FILE ] ; then
        echo "newedge files for $TOKDATE doesn't exist " >> $COMMISSIONS_RES_FILE
fi

#broker files are fetched now. Proceed to calculate commissions and recon with ORS data

echo "Getting ors commissions: "
$ORS_COMMISSION_SCRIPT $YYYYMMDD > $ORS_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE

echo "recon edf commissions: "
perl $EDF_COMMISSION_SCRIPT $YYYYMMDD >>$EDF_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
sed -i 's/~/ /g' $EDF_COMMISSIONS_FILE
python $COMPARE_COMMISSION_SCRIPT $EDF_COMMISSIONS_FILE $ORS_COMMISSIONS_FILE "EDF" $YYYYMMDD >> $COMMISSIONS_RES_FILE 2>>$COMMISSIONS_ERR_FILE

echo "recon newedge $YYYYMMDD"
perl $GENERAL_NEWEDGE_COMMISSION_SCRIPT $YYYYMMDD >> $NEWEDGE_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
sed -i 's/~/ /g' $NEWEDGE_COMMISSIONS_FILE
python $COMPARE_COMMISSION_SCRIPT $NEWEDGE_COMMISSIONS_FILE $ORS_COMMISSIONS_FILE "NEWEDGE" $YYYYMMDD >> $COMMISSIONS_RES_FILE 2>>$COMMISSIONS_ERR_FILE
cp $NEWEDGE_COMMISSIONS_FILE $BACKUP_COMMISSIONS_FILE
>$NEWEDGE_COMMISSIONS_FILE

echo "recon newedge cfe/ose/hk"
$ORS_COMMISSION_SCRIPT $TOKDATE > $ORS_COMMISSIONS_TOKDATE_FILE 2>>$COMMISSIONS_ERR_FILE
perl $CFE_COMMISSION_SCRIPT $TOKDATE >> $NEWEDGE_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
perl $OSE_COMMISSION_SCRIPT $TOKDATE >> $NEWEDGE_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
perl $HK_COMMISSION_SCRIPT $TOKDATE >> $NEWEDGE_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
sed -i 's/~/ /g' $NEWEDGE_COMMISSIONS_FILE
python $COMPARE_COMMISSION_SCRIPT $NEWEDGE_COMMISSIONS_FILE $ORS_COMMISSIONS_TOKDATE_FILE "NEWEDGE(C/O/HK)" $TOKDATE >> $COMMISSIONS_RES_FILE 2>>$COMMISSIONS_ERR_FILE
cat $NEWEDGE_COMMISSIONS_FILE >> $BACKUP_COMMISSIONS_FILE
mv $BACKUP_COMMISSIONS_FILE $NEWEDGE_COMMISSIONS_FILE

echo "recon bcs"
perl $BCS_COMMISSION_SCRIPT $YYYYMMDD >>$BCS_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
sed -i 's/~/ /g' $BCS_COMMISSIONS_FILE
python $COMPARE_COMMISSION_SCRIPT $BCS_COMMISSIONS_FILE $ORS_COMMISSIONS_FILE "BCS" $YYYYMMDD >> $COMMISSIONS_RES_FILE 2>>$COMMISSIONS_ERR_FILE

echo "recon plural"
perl $PLURAL_COMMISSION_SCRIPT $YYYYMMDD >> $PLURAL_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE
sed -i 's/~/ /g' $PLURAL_COMMISSIONS_FILE
python $COMPARE_COMMISSION_SCRIPT $PLURAL_COMMISSIONS_FILE $ORS_COMMISSIONS_FILE "PLURAL" $YYYYMMDD >> $COMMISSIONS_RES_FILE 2>>$COMMISSIONS_ERR_FILE

echo "recon abn"
DAY_OF_WEEK=`date -d $YYYYMMDD +%w`
if [ $DAY_OF_WEEK == 1 ] ; then
  LOOK_BACK=3
else
  LOOK_BACK=1
fi

LAST_DATE=`date -d "$YYYYMMDD $LOOK_BACK day ago" +'%Y%m%d'`;
ASX_SERVER="10.23.43.51";
YYYY_TRADE="find /spare/local/ORSlogs/ -name 'trades.$YYYYMMDD' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";
PREV_YYYY_TRADE="find /spare/local/ORSlogs/ -name 'trades.$LAST_DATE' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";

first_session_start_=`date -d "$LAST_DATE 22:32" +"%s"`; #GMT
first_session_end_=`date -d "$YYYYMMDD 06:30" +"%s"`; #GMT;
second_session_start_=`date -d "$LAST_DATE 07:12" +"%s"`; #GMT
second_session_end_=`date -d "$LAST_DATE 21:00" +"%s"`;

# get ASX ORS trade #
#get trades from current day's session 1
file=`ssh -o ConnectTimeout=20 dvcinfra@$ASX_SERVER $YYYY_TRADE | grep trades`
>$EACH_FILE
>$ASX_ORS_TRADE_FILE
rsync -avz --timeout=30 --quiet dvcinfra@$ASX_SERVER:$file $EACH_FILE
cat $EACH_FILE | tr '\001' ' ' | awk -v first_session_start_="$first_session_start_" -v first_session_end_="$first_session_end_"  \
	'{if($7>=first_session_start_ && $7<=first_session_end_) print $0}' | tr ' ' '\001' >> $ASX_ORS_TRADE_FILE

#get trades for previous day's session2
file=`ssh -o ConnectTimeout=20 dvcinfra@$ASX_SERVER $PREV_YYYY_TRADE | grep trades`
>$EACH_FILE
rsync -avz --timeout=30 --quiet dvcinfra@$ASX_SERVER:$file $EACH_FILE
cat $EACH_FILE | tr '\001' ' ' | awk -v second_session_start_="$second_session_start_" -v second_session_end_="$second_session_end_" \
      '{if($7>=second_session_start_ && $7<=second_session_end_) print $0}' | tr ' ' '\001' >> $ASX_ORS_TRADE_FILE

#ors asx commissions
perl $PNL_SCRIPT "P" $ASX_ORS_TRADE_FILE $YYYYMMDD > $ORS_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE

#get abnamro trades
perl $ABN_COMMISSION_SCRIPT $YYYYMMDD > $ABN_COMMISSIONS_FILE 2>>$COMMISSIONS_ERR_FILE

#compare broker and ors data
python $COMPARE_COMMISSION_SCRIPT $ABN_COMMISSIONS_FILE $ORS_COMMISSIONS_FILE "ABN" $YYYYMMDD >> $COMMISSIONS_RES_FILE 2>>$COMMISSIONS_ERR_FILE

rm -f $ASX_ORS_TRADE_FILE

#email the discrepancies, if any
allowed_commission_difference=50
awk -v threshold="$allowed_commission_difference" 'function abs(x){return ((x < 0.0) ? -x : x)} {if(NF==4 && abs($4)>=50) print $0}' $COMMISSIONS_RES_FILE
