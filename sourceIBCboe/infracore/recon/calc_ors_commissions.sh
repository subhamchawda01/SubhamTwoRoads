#!/bin/bash

usage="calc_ors_commissions.sh YYYYMMDD [dump_eod_position]"

if [[ ( $# -ne 1 ) && ( $# -ne 2 ) ]] ;
then
    echo -e $usage;
    exit;
fi

YYYYMMDD=$1;

EACH_FILE="$HOME/abhishek_temp/EACH_TRADE"
FILE="$HOME/abhishek_temp/ALL_TRADES";
RETAIL_FILE="$HOME/EODPnl/RETAIL_TRADES";
PNL_SCRIPT="$HOME/abhishek_temp/recon/see_ors_pnl.pl";
SERVERS_SCRIPT="$HOME/infracore/GenPerlLib/print_all_machines_vec.pl"

> $EACH_FILE
> $FILE;
> $RETAIL_FILE;

PREV_DATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;

GUI_TRADES_FILE=$HOME/EODPnl/gui_trades.$YYYYMMDD;
if [ -e $GUI_TRADES_FILE ] ; then 
    cat $GUI_TRADES_FILE >> $FILE ;
fi

#DC-drop copy, FFDVC - CFE, EJG9 - LIFFE , DBRP - BMF
command="find /spare/local/ORSlogs/ -name 'trades.$YYYYMMDD' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";

servers=(`perl $SERVERS_SCRIPT`);

for server in "${servers[@]}"
{
    [[ "$server" =~ ^#.*$ ]] && continue
    
    files=`ssh -o ConnectTimeout=20 dvcinfra@$server $command | grep trades`
    files_array=($files)
    for trade_file in "${files_array[@]}"
    {
        >$EACH_FILE
        rsync -avz --timeout=30 --quiet dvcinfra@$server:$trade_file $EACH_FILE    
        cat $EACH_FILE >> $FILE
    }
}

perl -w $PNL_SCRIPT 'P' $FILE $YYYYMMDD 2> /dev/null

