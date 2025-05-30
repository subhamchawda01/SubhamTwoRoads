#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 SHORTCODE DATE(YYYYMMDD) TIME_PERIOD(sec) SUMMARY/PLOT PLOT_OPTIONS(AVC/VR/TVC/TVR/ATC/TR/VGC)"
EXAMP1="$0 FESX_0 20120421 3600 PLOT VR"
NOTE1="AVC : Absolute Volume Comparison Over Each Time-Frame( Our Volumes And Exchnage Volumes )\nVR  : Our Volumes / Exch Volumes Over Each Time Frame Ratio"
NOTE2="TVC : Absolute Total Volume Comparison ( Our Volumes And Exchnage Volumes )\nTVR  : Our Total Volumes / Exch Total Volumes Ratio"
NOTE3="ATC : Absolute No.Of Trades Comparison ( Our No.Of Trades And Exchange No.of Trades )\nTR   : Our No.of Trades / Exch No.of Trades Over Each Time-Frame Ratio"
NOTE4="VGC : Our Volume Growth Against Exchange Volume Growth over period" 

if [ $# -lt 4 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    echo "Where, " ;
    echo -e $NOTE1;
    echo -e $NOTE2;
    echo -e $NOTE3;
    echo $NOTE3;
    exit;
fi

SHORTCODE=$1 ;
YYYYMMDD=$2 ;
PERIOD=$3 ;
OUT_FRMT=$4 ;

if [ $OUT_FRMT == "PLOT" ]
then

    PLOT_OPTION=$5 ;

fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


#============================  QueryTradesLog & Stats Exec ==================================#

OUR_TRADES_DIR=/NAS1/logs/QueryTrades

#SHORTCODE_SYMBOL_EXEC=$HOME/LiveExec/bin/get_exchange_symbol
#OUR_PRESENCE_IN_MARKET_STATS_EXEC=$HOME/LiveExec/bin/get_our_presence_in_market_stats

SHORTCODE_SYMBOL_EXEC=$HOME/basetrade_install/bin/get_exchange_symbol
OUR_PRESENCE_IN_MARKET_STATS_EXEC=$HOME/basetrade_install/bin/get_our_presence_in_market_stats

#============================================================================================#


#============================= Get Symbol =====================================#

SYMBOL=`$SHORTCODE_SYMBOL_EXEC $SHORTCODE $YYYYMMDD | tr ' ' '~'` ;

if [ $SYMBOL = "kGetContractSpecificationMissingCode" ]
then 
    
    echo $SYMBOL ;
    exit ;

fi

#============================================================================================#


#================================= Store Stats And Volume/Trades Files ==========================#

OUR_TEMP_TRADES_FILE=/tmp/our_trades_$SYMBOL"_"$YYYYMMDD".txt"
OUR_TEMP_TRADES_FMT_FILE=/tmp/our_trades_$SYMBOL"_"$YYYYMMDD"_fmt.txt"

OUR_PRESENCE_IN_MARKET_STATS_FILE=/tmp/our_presence_in_market_stats_$SYMBOL"_"$YYYYMMDD".txt"

grep -w $SYMBOL $OUR_TRADES_DIR/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/trades.* | egrep "OPEN|FLAT" > $OUR_TEMP_TRADES_FILE 

cat $OUR_TEMP_TRADES_FILE | tr ':' ' ' | tr '.' ' ' | awk '{print $4 " " $10}' | sort -nk 1 > $OUR_TEMP_TRADES_FMT_FILE

echo $OUR_PRESENCE_IN_MARKET_STATS_EXEC $OUR_TEMP_TRADES_FMT_FILE $SHORTCODE $YYYYMMDD $PERIOD
$OUR_PRESENCE_IN_MARKET_STATS_EXEC $OUR_TEMP_TRADES_FMT_FILE $SHORTCODE $YYYYMMDD $PERIOD > $OUR_PRESENCE_IN_MARKET_STATS_FILE

#============================================================================================#

#=========================================  Dump Stats =======================================#

if [ $OUT_FRMT == "SUMMARY" ] 
then 

    cat $OUR_PRESENCE_IN_MARKET_STATS_FILE ;
#============================== Clean Up =================================#

rm -f $OUR_PRESENCE_IN_MARKET_STATS_FILE
rm -f $OUR_TEMP_TRADES_FILE
rm -f $OUR_TEMP_TRADES_FMT_FILE

#===========================================================================
    exit ;

fi

#============================================================================================#


#========================================== Stats Offsets =====================================#

START_TIME_OFFSET=1
END_TIME_OFFSET=2
OUR_PERI_VOL_OFFSET=3
EXCH_PERI_VOL_OFFSET=4
VOL_RATIO_OFFSET=5
OUR_TOT_VOL_OFFSET=6
OUR_VOL_GROWTH_OFFSET=7
EXCH_TOT_VOL_OFFSET=8
EXCH_VOL_GROWTH_OFFSET=9
OUR_TRADES_OFFSET=10
EXCH_TRADES_OFFSET=11

#============================================================================================#


#========================================= PLOT ======================================#

GNUPLOT_CMD="";

OUR_TOTAL_VOLUME_=`tail -1 $OUR_PRESENCE_IN_MARKET_STATS_FILE | awk '{print $6}' | bc`
EXCH_TOTAL_VOLUME_=`tail -1 $OUR_PRESENCE_IN_MARKET_STATS_FILE | awk '{print $8}' | bc`

EXCH_VOL_BALANCE_RATIO_=`echo $EXCH_TOTAL_VOLUME_/$OUR_TOTAL_VOLUME_ | bc`

# reducing the scale down factor by 10, so our volumes spike should not corss exch volumes in most cases
EXCH_VOL_BALANCE_RATIO_=`echo $EXCH_VOL_BALANCE_RATIO_ - 10 | bc`  

case $PLOT_OPTION in

    AVC) 

	GNUPLOT_CMD='set title "Our Presence in Market( Volumes )"; set xlabel "Time ( UTC )"; set ylabel "Volumes"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':'$OUR_PERI_VOL_OFFSET' title "Our Volumes" with lines, "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':($'$EXCH_PERI_VOL_OFFSET'/'$EXCH_VOL_BALANCE_RATIO_') title "Exchange Volumes ( 1/'$EXCH_VOL_BALANCE_RATIO_' )" with lines'

	;;

    VR)

	GNUPLOT_CMD='set title "Our Presence in Market( VolumesRatio )"; set xlabel "Time ( UTC )"; set ylabel "Our Volumes To Exch Volumes Ratio(%)"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':($'$OUR_PERI_VOL_OFFSET'/$'$EXCH_PERI_VOL_OFFSET')*100 title "Volume Ratio(%) ( Ours/Exch) " with lines'

	;;

    TVC) 

	GNUPLOT_CMD='set title "Our Presence in Market( TotalVolumes )"; set xlabel "Time ( UTC )"; set ylabel "Volumes"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':'$OUR_TOT_VOL_OFFSET' title "Our Total Volumes" with lines, "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':($'$EXCH_TOT_VOL_OFFSET'/'$EXCH_VOL_BALANCE_RATIO_') title "Exchange Total Volumes ( 1/'$EXCH_VOL_BALANCE_RATIO_' )" with lines'

	;;

    TVR)

	GNUPLOT_CMD='set title "Our Presence in Market( TotalVolumesRatio )"; set xlabel "Time ( UTC )"; set ylabel "Our Volumes To Exch Volumes Ratio(%)"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':($'$OUR_TOT_VOL_OFFSET'/$'$EXCH_TOT_VOL_OFFSET')*100 title "Total Volumes Ratio(%) ( Ours/Exch) " with lines'

	;;

    ATC) 
	
	GNUPLOT_CMD='set title "Our Presence in Market( No.Of Trades )"; set xlabel "Time ( UTC )"; set ylabel "Number Of Trades"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':'$OUR_TRADES_OFFSET' title "Our No.of Trades" with lines, "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':'$EXCH_TRADES_OFFSET' title "Exchange No.of Trades" with lines'

	;;

    TR) 

	GNUPLOT_CMD='set title "Our Presence in Market( No.Of Trades )"; set xlabel "Time ( UTC )"; set ylabel "Our Trades TO Exch Trades Ratio(%)"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':($'$OUR_TRADES_OFFSET'/$'$EXCH_TRADES_OFFSET')*100 title "No.Of Trades Ratio(%) ( Ours/Exch )" with lines'

	;;

    VGC)

	GNUPLOT_CMD='set title "Our Presence in Market( Volume Growth )"; set xlabel "Time ( UTC )"; set ylabel "Volume Growth % ( Ours & Exch )"; set xdata time; set timefmt "%s"; plot "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':'$OUR_VOL_GROWTH_OFFSET' title "Our Volume Growth( % )" with lines, "'$OUR_PRESENCE_IN_MARKET_STATS_FILE'" using '$END_TIME_OFFSET':'$EXCH_VOL_GROWTH_OFFSET' title "Exchange Volume Growth( % )" with lines'
	
	;;

    *) 

	echo "Invalid PLOT OPTION" ;
	echo $USAGE1;  
	echo $EXAMP1;
#============================== Clean Up =================================#

rm -f $OUR_PRESENCE_IN_MARKET_STATS_FILE
rm -f $OUR_TEMP_TRADES_FILE
rm -f $OUR_TEMP_TRADES_FMT_FILE

#===========================================================================
	exit ;

	;;

esac

echo $GNUPLOT_CMD | gnuplot -persist

#============================================================================================#

#============================== Clean Up =================================#

rm -f $OUR_PRESENCE_IN_MARKET_STATS_FILE
rm -f $OUR_TEMP_TRADES_FILE
rm -f $OUR_TEMP_TRADES_FMT_FILE

#===========================================================================
exit ;
