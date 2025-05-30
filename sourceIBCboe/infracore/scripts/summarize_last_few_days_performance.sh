#!/bin/bash

NUM_DAYS_PAST=21;
if [ $# -gt 0 ] ; then
    NUM_DAYS_PAST=$1; shift;
fi

echo "Performance over last $NUM_DAYS_PAST days";

printf "%s\t" "SHCODE"
printf "%s\t" "SUMPNL"
printf "%s\t" "AVG"
printf "%s\t" "STD"
printf "%s\t" "SHRP";
printf "%s\t" "AVGVOL";
printf "%s" "MAXDRAWDOWN";
printf "\n";

for SHORTCODE in  ALL CME EUREX TMX ZF ZN ZB UB FGBS FGBM FGBL FESX FDAX CGB CGF CGZ BAX SXF BR_DOL_0 BR_WDO_0 BR_IND_0 BR_WIN_0 ;
do
    SUMOUT=`~/infracore/scripts/query_pnl_data_mult.pl TODAY-$NUM_DAYS_PAST TODAY $SHORTCODE | awk -F, '{print $2}' | ~/infracore/scripts/sumcalc.pl | awk '{printf "%s", $1}'`;
    if [ $SUMOUT -ne 0 ] ; then
	SCOUT=`~/infracore/scripts/query_pnl_data_mult.pl TODAY-$NUM_DAYS_PAST TODAY $SHORTCODE | awk -F, '{print $2}' | ~/infracore/scripts/statcalcsinglevec.pl | awk '{ if ( NF >= 3 ) { printf "%d %d %.2f", $1,$2, $3} }'`;
	VOLMEAN=`~/infracore/scripts/query_pnl_data_mult.pl TODAY-$NUM_DAYS_PAST TODAY $SHORTCODE | awk -F, '{print $3}' | ~/infracore/scripts/statcalcsinglevec.pl | awk '{ if ( NF >= 1 ) { printf "%d", $1} }'`;
	DRAWD=`~/infracore/scripts/query_pnl_data_mult.pl TODAY-$NUM_DAYS_PAST TODAY $SHORTCODE | awk -F, '{print $2}' | ~/infracore/scripts/get_max_drawdown_singlevec.pl | awk '{ if ( NF >= 1 ) { printf "%d", $1} }'`;
	printf "%s\t" $SHORTCODE;
	printf "%s\t" $SUMOUT;
	printf "%s\t" $SCOUT;
	printf "%s\t" $VOLMEAN;
	printf "%s" $DRAWD;
	printf "\n";
    fi
done
