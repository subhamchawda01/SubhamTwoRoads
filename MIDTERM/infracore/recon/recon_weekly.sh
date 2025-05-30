#!/bin/bash

USAGE="$0 YYYYMMDD (it should be a saturday)\n\t >> recon for week ending on this date for date YYYYMMDD.";

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi

YYYYMMDD=$1;

NEXT_BUSINESS_DAY_SCRIPT=$HOME/infracore/GenPerlLib/get_prev_or_next_business_day.pl

GET_YESTERDAY_EXEC="/home/dvcinfra/infracore_install/bin/calc_prev_day";
PNL_DIR="/home/dvcinfra/recon";
TEMP_DIR="/home/dvcinfra/temp";
SERVERS_SCRIPT="$HOME/infracore/GenPerlLib/print_all_machines_vec.pl";
ors_trades_file="$PNL_DIR/ors_trades";
broker_trades_file="$PNL_DIR/broker_trades";

FETCH_NEWEDGE_SCRIPT=$HOME/infracore/recon/fetch_newedge_files.sh
FETCH_BCS_SCRIPT=$HOME/infracore/recon/fetch_bcs_files.sh
FETCH_PLURAL_SCRIPT=$HOME/infracore/recon/fetch_plural_files.sh
GET_NEWEDGE_TRADES=$HOME/infracore/recon/get_general_newedge_eod_trades_in_ors_format.pl
GET_OSE_TRADES=$HOME/infracore/recon/get_ose_newedge_eod_trades_in_ors_format.pl
GET_CFE_TRADES=$HOME/infracore/recon/get_cfe_newedge_eod_trades_in_ors_format.pl
GET_HK_TRADES=$HOME/infracore/recon/get_hk_newedge_eod_trades_in_ors_format.pl
GET_BCS_TRADES=$HOME/infracore/recon/get_general_bcs_eod_trades_in_ors_format.pl
GET_PLURAL_TRADES=$HOME/infracore/recon/get_general_plural_eod_trades_in_ors_format.pl


#OVER_WEEKEND_POSITION_FILE="/spare/local/files/EOWPositions/overweekend_positions_$YYYYMMDD";
OVER_WEEKEND_POSITION_FILE="$HOME/sasidhar/overweekend_positions";

OVER_WEEKEND_POSITION_FILE_NEW="$HOME/sasidhar/overweekend_positions_1";
#OVER_WEEKEND_POSITION_FILE_NEW="/spare/local/files/EOWPositions/overweekend_positions_$YYYYMMDD(- 7 days)";

#COMPARE_POSITION_SCRIPT=$HOME/infracore/scripts/compare_position.pl
COMPARE_POSITION_SCRIPT=$HOME/sasidhar/compare_position.pl

weekly_broker_trades="$PNL_DIR/weekly_broker_trades";
weekly_ors_trades="$PNL_DIR/weekly_ors_trades";
each="$PNL_DIR/each";

>$weekly_broker_trades;
>$weekly_ors_trades;
>$each

mkdir -p $TEMP_DIR
cd $TEMP_DIR

generate_broker_trades () {
	date=$1
	echo "Getting broker trades on $date";
	
	$FETCH_NEWEDGE_SCRIPT $date
	$FETCH_BCS_SCRIPT $date
	$FETCH_PLURAL_SCRIPT $date
	
	BCS_TRADE_FILE="/apps/data/BCSTrades/BCSFiles/"$date"_trades";
	NEWEDGE_TRADE_FILE="/apps/data/MFGlobalTrades/MFGFiles/GMITRN_"$date".csv"
	PLURAL_TRADE_FILE="/apps/data/PluralTrades/PluralFiles/InvoiceDetailed_$date.csv";
	
	perl $GET_OSE_TRADES $date >> $weekly_broker_trades
	perl $GET_CFE_TRADES $date >> $weekly_broker_trades
	perl $GET_HK_TRADES $date >> $weekly_broker_trades
	perl $GET_NEWEDGE_TRADES $NEWEDGE_TRADE_FILE $date >> $weekly_broker_trades
	perl $GET_BCS_TRADES $BCS_TRADE_FILE $date >> $weekly_broker_trades
	perl $GET_PLURAL_TRADES $PLURAL_TRADE_FILE $date >> $weekly_broker_trades
}

generate_ors_trades (){
    date=$1
    
    echo "Getting ors trades on $date";
	
	GUI_TRADES_FILE=$HOME/EODPnl/gui_trades.$date;
	if [ -e $GUI_TRADES_FILE ] ; then 
	    cat $GUI_TRADES_FILE >>  $weekly_ors_trades;
	fi
	
	#DC-drop copy, FFDVC - CFE, EJG9 - LIFFE , DBRP - BMF
	command="find /spare/local/ORSlogs/ -name 'trades.$date' | grep -v DC | grep -v FFDVC | grep -v DBRP | grep -v EJG9";
	
	servers=(`perl $SERVERS_SCRIPT`);
	
	for server in "${servers[@]}"
	{
	    [[ "$server" =~ ^#.*$ ]] && continue
	    
	    files=`ssh -o ConnectTimeout=20 dvcinfra@$server $command | grep trades`
	    files_array=($files)
	    for trade_file in "${files_array[@]}"
	    {
	        >$each
	        rsync -avz --timeout=30 --quiet dvcinfra@$server:$trade_file $each
	        cat $each >> $weekly_ors_trades;
	    }
	}
}


current_date=$YYYYMMDD;

#for ose and cfe
next_date=`perl $NEXT_BUSINESS_DAY_SCRIPT 1 $current_date`; 
echo $next_date;
$FETCH_NEWEDGE_SCRIPT $next_date

for i in `seq 1 2`;
do
   current_date=`$GET_YESTERDAY_EXEC $current_date`
   generate_ors_trades $current_date
   generate_broker_trades $current_date
done

sed -i 's/~/ /g' $weekly_broker_trades
perl $COMPARE_POSITION_SCRIPT $weekly_broker_trades $weekly_ors_trades 0 $OVER_WEEKEND_POSITION_FILE > ~/sasidhar/result
#result has to be mailed
perl $COMPARE_POSITION_SCRIPT $weekly_broker_trades $weekly_ors_trades 1 $OVER_WEEKEND_POSITION_FILE > $OVER_WEEKEND_POSITION_FILE_NEW

rm -r $TEMP_DIR