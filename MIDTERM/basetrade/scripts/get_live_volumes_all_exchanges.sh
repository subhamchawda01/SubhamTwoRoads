USAGE1="$0 BASE_SYMBOL_FILE YYYYMMDD[TODAY]"
EXAMP1="$0 ~/infracore_install/files/volume_based_symbols_to_eval.txt 20111230" 

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

BASE_SYM_FILE=$1
YYYYMMDD=$2;

if [ $YYYYMMDD = "TODAY" ] ;
then
  YYYYMMDD=$(date "+%Y%m%d")
fi

EXEC=$HOME/LiveExec/bin/all_exchanges_volume_reconciliation
LOGDIR=/spare/local/VolumeBasedSymbol
LOGFILE=current_volume_sorted.txt

#clears out old volumes
>$LOGDIR/$LOGFILE
touch /tmp/$LOGFILE
>/tmp/$LOGFILE

#get current volumes
ssh dvcinfra@10.23.200.52 '/home/dvcinfra/LiveExec/scripts/all_mds_volumes_of_day.sh EUREX '$YYYYMMDD'' | tr '/' ' ' | tr '_' ' ' | awk '{print $5 " " $7}' >> /tmp/$LOGFILE
ssh dvcinfra@10.23.196.52 '/home/dvcinfra/LiveExec/scripts/all_mds_volumes_of_day.sh CME '$YYYYMMDD'' | tr '/' ' ' | tr '_' ' ' | awk '{print $5 " " $7}' >> /tmp/$LOGFILE
ssh dvcinfra@10.23.182.51 '/home/dvcinfra/LiveExec/scripts/all_mds_volumes_of_day.sh TMX '$YYYYMMDD'' | tr '/' ' ' | tr '_' ' ' | awk '{print $6 " " $8}' >> /tmp/$LOGFILE
ssh dvcinfra@10.23.23.11 '/home/dvcinfra/LiveExec/scripts/all_mds_volumes_of_day.sh NTP '$YYYYMMDD'' | tr '/' ' ' | tr '_' ' ' | awk '{print $5 " " $7}' >> /tmp/$LOGFILE

#sort by volumes
cat /tmp/$LOGFILE | sort -n -k 2 > $LOGDIR/$LOGFILE
rm /tmp/$LOGFILE

$EXEC $BASE_SYM_FILE $YYYYMMDD 
