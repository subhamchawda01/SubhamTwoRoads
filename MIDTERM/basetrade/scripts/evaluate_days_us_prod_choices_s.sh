#!/bin/bash

dstr=`date +%Y%m%d`; 
SHORTCODE=ZN_0;
if [ $# -gt 1 ] ; then
    SHORTCODE=$1; shift;
    dstr=$1; shift;
else
    echo "USAGE: PROD YYYYMMDD";
    exit;
fi

TEMPSTRATFILE=$HOME/choices/t_file.txt;
BASE_UNIQUE_SIM_ID=72721;

# by sharpe
    CHOICEFILE=$HOME/choices/US_MORN_DAY_s_$SHORTCODE.$dstr;
    if [ -e $CHOICEFILE ] ; then
	rm -f $TEMPSTRATFILE ;
	UNIQUEID=$BASE_UNIQUE_SIM_ID;
	for name in `head $CHOICEFILE | awk '{print $3}'`; 
	do 
	    echo $name;
	    FULLSTRATFILE=~/modelling/strats/*/*/$name
	    if [ -e $FULLSTRATFILE ] ; then
		cat $FULLSTRATFILE | awk '{$8='$UNIQUEID'; print $0}' >> $TEMPSTRATFILE ; 
	    fi
	    UNIQUEID=$(( $UNIQUEID + 1 ));
	done

	if [ $UNIQUEID -gt $BASE_UNIQUE_SIM_ID ]; then
	    ~/basetrade/scripts/run_mult_strats_and_plot.sh $TEMPSTRATFILE $dstr;
	fi
    fi
