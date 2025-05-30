#!/bin/bash

dstr=`date +%Y%m%d`; 

if [ $# -gt 0 ] ; then
    dstr=$1; shift;
fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


TEMPSTRATFILE=$HOME/choices/t_file.txt;
BASE_UNIQUE_SIM_ID=72721;

for SHORTCODE in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FESX_0 FDAX_0 ; 
do 
# by sharpe
    CHOICEFILE=$HOME/choices/EU_MORN_DAY_p_$SHORTCODE.$dstr;
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
	    ~/infracore/scripts/run_mult_strats_and_plot.sh $TEMPSTRATFILE $dstr;
	fi
    fi

done
