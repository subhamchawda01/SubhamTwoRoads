#!/bin/bash

dstr=`date +%Y%m%d`; 

if [ $# -gt 0 ] ; then
    dstr=$1; shift;
fi

for SHORTCODE in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FESX_0 FDAX_0 ; 
do 
# by pnl
    CHOICEFILE=$HOME/choices/EU_MORN_DAY_p_$SHORTCODE.$dstr;
    if [ -e $CHOICEFILE ] ; then
	STRATNAME=`head -1 $CHOICEFILE | awk '{ print $3; }'`;
	if [ "$STRATNAME" != "" ]; then
#	    echo "$SHORTCODE $STRATNAME :";
	    EXP_PNL=`head -1 $CHOICEFILE | awk '{ print $4; }'`;
	    EXP_VOL=`head -1 $CHOICEFILE | awk '{ print $6; }'`;
	    echo "$SHORTCODE $STRATNAME";
	    echo "EXPECTED_PNL_VOLUME $EXP_PNL, $EXP_VOL";
	    ~/basetrade/scripts/run_strat_and_plot.sh $STRATNAME $dstr;
	fi
    fi

done