#!/bin/bash

#do not run this file manually
FORMAT="script mode [YYYYMMDD] [EOD](dump_EOD_position and store EOD pnls) : do not run this file manually"
if [ $# -lt 1 ] ;
then
    echo $FORMAT;
    exit 0;
fi

MODE=$1;

YYYYMMDD=$(date --date='30 minutes' "+%Y%m%d");


if [ $# -ge 2 ] ;
then
    YYYYMMDD=$2;
fi

if [ $# -ge 3 ] ;
then
    EOD=$3;
fi


if [[ $MODE -ne 'C' && $MODE -ne 'R' && $MODE -ne 'E' ]] 
then
	echo "MODE has to be C/R/E. Exiting..."
	exit 0;
fi

GENERAL_OUT_FILE_C="$HOME/PrintPnl/pnls_now_c"
GENERAL_OUT_FILE_R="$HOME/PrintPnl/pnls_now_r"
GENERAL_OUT_FILE_E="$HOME/PrintPnl/pnls_now_e"
OUT_PRODUCED="$HOME/PrintPnl/out_produced"


if [ -f $GENERAL_OUT_FILE_E ] 
then
	rm $OUT_PRODUCED
	rm $GENERAL_OUT_FILE_E
	rm $GENERAL_OUT_FILE_R
	rm $GENERAL_OUT_FILE_C
fi

EACH_FILE="$HOME/PrintPnl/EACH_TRADE"
FILE="$HOME/PrintPnl/ALL_TRADES";
RETAIL_FILE="$HOME/EODPnl/RETAIL_TRADES";
PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";
PNL_SCRIPT_RETAIL="$HOME/infracore_install/scripts/see_retail_pnl_nocolor.pl";
DUMP_OVN_PNL_SCRIPT="$HOME/infracore_install/scripts/see_ors_pnl.pl";
TMX_POS_COMPUTE="/home/dvcinfra/LiveExec/scripts/compute_tmx_positions.pl";
SERVERS_SCRIPT="$HOME/infracore/GenPerlLib/print_all_machines_vec.pl"
EOD_OUT_FILE="/apps/data/MFGlobalTrades/EODPnl/ors_pnls_$YYYYMMDD.txt";
EOD_CHECK_FILE="$HOME/PrintPnl/eod_check_file"

GUI_TRADES_FILE=$HOME/EODPnl/gui_trades.$YYYYMMDD;
EOD=0;

if [ ! -d $HOME/PrintPnl ] ;
then
    mkdir $HOME/PrintPnl;
fi

> $FILE;
> $RETAIL_FILE;

PREV_DATE=`$HOME/infracore_install/bin/calc_prev_week_day $YYYYMMDD` ;


if [ -e $GUI_TRADES_FILE ] ; 
then 
	cat $GUI_TRADES_FILE >> $FILE ;
fi

#DC-drop copy, FFDVC - CFE, EJG9 - LIFFE , DBRP - BMF
command="find /spare/local/ORSlogs/ -name 'trades.$YYYYMMDD' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";

servers=(`perl $SERVERS_SCRIPT`);

for server in "${servers[@]}"
{
	[[ "$server" =~ ^#.*$ ]] && continue
	
	files=`timeout 30s ssh -o ConnectTimeout=20 dvcinfra@$server $command | grep trades`
	files_array=($files)
	
	for trade_file in "${files_array[@]}"
	{
		>$EACH_FILE
		rsync -avz --timeout=60 --quiet dvcinfra@$server:$trade_file $EACH_FILE    
		cat $EACH_FILE >> $FILE
	}
}

twime_trade="find /home/dvcinfra/twime_test -name 'trades.$YYYYMMDD' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";
trade_file=`ssh -o ConnectTimeout=20 dvcinfra@172.18.244.107 $twime_trade| grep trades`
>$EACH_FILE
rsync -avz --timeout=30 --quiet dvcinfra@172.18.244.107:$trade_file $EACH_FILE
cat $EACH_FILE >> $FILE
perl -w $PNL_SCRIPT 'C' $FILE $YYYYMMDD >$GENERAL_OUT_FILE_C 2>/dev/null &
perl -w $PNL_SCRIPT 'R' $FILE $YYYYMMDD >$GENERAL_OUT_FILE_R 2>/dev/null &
perl -w $PNL_SCRIPT 'E' $FILE $YYYYMMDD >$GENERAL_OUT_FILE_E 2>/dev/null
sleep 10;

if [ -e $EOD_CHECK_FILE ]
then
	EOD=1;
	rm $EOD_CHECK_FILE
fi

if [ $EOD -eq 1 ]; 
then
	cat $GENERAL_OUT_FILE_R > ${EOD_OUT_FILE};
    perl -w $DUMP_OVN_PNL_SCRIPT 'R' $FILE $YYYYMMDD 1 > /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt 2>/dev/null 
    perl -w $TMX_POS_COMPUTE /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp.txt > /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp2.txt 2>/dev/null; 
    cp /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp2.txt /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD".txt    
    rm /spare/local/files/EODPositions/overnight_pnls_"$YYYYMMDD"_tmp2.txt 2>/dev/null;
    
    #sync to file-server 
	scp $OUT_FILE dvcinfra@10.23.74.40:/apps/data/MFGlobalTrades/EODPnl/ >/dev/null 2>/dev/null
fi

touch $OUT_PRODUCED
sleep 2; #to allow files which are waiting to read	 
