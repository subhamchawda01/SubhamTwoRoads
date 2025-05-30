#!/bin/bash

LOCKFILE=$HOME/locks/getpnls.lock;
if [ -e $LOCKFILE ] ; then
    date;
    ls -lrt $LOCKFILE;
    echo "$LOCKFILE present. Please delete";
    exit;
fi

touch $LOCKFILE;

usage="get_EOD_pnl.sh YYYYMMDD [dump_eod_position]"

if [[ ( $# -ne 1 ) && ( $# -ne 2 ) ]] ;
then
    echo "$usage";
    rm $LOCKFILE;
    exit;
fi

YYYYMMDD=$1;

EACH_FILE="$HOME/EODPnl/EACH_TRADE"
DELTA_REC_DIR="/spare/local/logs/pnl_data/hft/delta_recovery/"
PNL_SCRIPT="/home/pengine/prod/live_scripts/show_pnl_generic_tagwise.pl";
SERVERS_SCRIPT="$HOME/infracore/GenPerlLib/print_all_machines_vec.pl"
EOD_FILE_LOCATION="/spare/local/files/EODPositions";

if [ ! -d $HOME/EODPnl ] ;
then
    mkdir $HOME/EODPnl;
fi

#DC-drop copy, FFDVC - CFE, EJG9 - LIFFE , DBRP - BMF
command="find /spare/local/ORSlogs/ -name 'trades.$YYYYMMDD' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";

servers=(`perl $SERVERS_SCRIPT`);

for server in "${servers[@]}"
{
    [[ "$server" =~ ^#.*$ ]] && continue
    
    files=`ssh -o ConnectTimeout=20 dvcinfra@$server $command | grep trades`
    host_command="hostname"
    host=`ssh -o ConnectTimeout=20 dvcinfra@$server $host_command| awk -F "." '{print $1}'`
    FILE="$DELTA_REC_DIR""delta_"$host"_"$YYYYMMDD"123"
    touch $FILE;
    > $FILE;
    files_array=($files)
    for trade_file in "${files_array[@]}"
    {
        >$EACH_FILE
        rsync -avz --timeout=30 --quiet dvcinfra@$server:$trade_file $EACH_FILE    
        cat $EACH_FILE >> $FILE
    }
}

cp /spare/local/logs/risk_logs/qid_saci_tag."$YYYYMMDD" /spare/local/logs/pnl_data/hft/tag_pnl/saci_maps_hist/qid_saci_tag_"$YYYYMMDD"
perl $PNL_SCRIPT C H $YYYYMMDD TT R 2>/dev/null

rm -f $LOCKFILE;
