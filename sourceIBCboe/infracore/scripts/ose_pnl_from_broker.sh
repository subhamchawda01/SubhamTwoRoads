#!/bin/bash

FORMAT="script mode"
if [ $# -lt 1 ] ;
then
    echo $FORMAT;
    exit 0;
fi

MODE=$1;
if [[ $MODE -ne 'C' && $MODE -ne 'R' && $MODE -ne 'E' ]] 
then
	echo "MODE has to be C/R/E. Exiting..."
	exit 0;
fi

#$GENERATE_INPUT_SCRIPT="$HOME/infracore/scripts/generate_input_for_see_pnls_from_broker_files.pl";
GENERATE_INPUT_SCRIPT=$HOME/infracore/scripts/generate_input_for_see_pnls_from_broker_files.pl
INPUT_FILE_FOR_SEE_ORS_PNL=$HOME/trades/ne_broker_interaday_trades_file
PNL_SCRIPT=$HOME/infracore_install/scripts/see_ors_pnl.pl;
#require "$HOME_DIR/$REPO/scripts/generate_input";



mkdir -p /apps/data/IntradayBrokerFiles/NewEdge;
cd /apps/data/IntradayBrokerFiles/NewEdge;

HOST='Ftp.newedgegroup.com';
USER='DVCAPITAL@ACC';
PASSWD='Newedge1009';
	
while [ true ]
do
	YYYYMMDD=$(date "+%Y%m%d")
	INTERADAY_FILE=$YYYYMMDD"_DV\\ CAPITAL_2.csv";
	INTERADAY_FILE_1=$YYYYMMDD"_DV CAPITAL_2.csv";

	TIME=$(date +"%I%M")

	echo "Downloading ose intraday broker trade file at $TIME ..."

ftp -n $HOST << SCRIPT
user $USER $PASSWD
binary
get $INTERADAY_FILE
quit
SCRIPT
	
	#cd /apps/data/IntradayBrokerFiles/NewEdge;
	if [ ! -f "$INTERADAY_FILE_1" ]; then
	echo "Download of ose intraday broker trade file at $TIME FAILED."
		sleep 10m
		continue;
	fi
	
	echo "Download of ose intraday broker trade file at $TIME SUCCESSFUL."
	
	FILE="trades_"$YYYYMMDD"_"$TIME".csv"
	cat "$INTERADAY_FILE_1" > $FILE
	
	> $INPUT_FILE_FOR_SEE_ORS_PNL;
	# run perl script
	perl $GENERATE_INPUT_SCRIPT "$INTERADAY_FILE_1" > $INPUT_FILE_FOR_SEE_ORS_PNL
	
	perl -w $PNL_SCRIPT $MODE $INPUT_FILE_FOR_SEE_ORS_PNL | grep -v INVALI
	
	sleep 10m
done
