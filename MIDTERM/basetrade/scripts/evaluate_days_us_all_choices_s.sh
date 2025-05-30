#!/bin/bash

dstr=`date +%Y%m%d`; 

if [ $# -gt 0 ] ; then
    dstr=$1; shift;
fi

TEMPSTRATFILE=$HOME/choices/t_file.txt;
BASE_UNIQUE_SIM_ID=72721;

NUM_FILES=1;
MKT_MODEL=2;
for SHORTCODE in ZT_0 ZF_0 ZN_0 ZB_0 UB_0 FGBS_0 FGBM_0 FGBL_0 FGBX_0 FESX_0 FDAX_0 CGB_0 SXF_0 BAX_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BAX_6 ; 
do 

    case $SHORTCODE
	in
	ZT_0)
	    NUM_FILES=0;
	    MKT_MODEL=1;
	    ;;
	ZF_0)
	    NUM_FILES=6;
	    MKT_MODEL=1;
	    ;;
	ZN_0)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	ZB_0)
	    NUM_FILES=1;
	    MKT_MODEL=2;
	    ;;
	FGBS_0)
	    NUM_FILES=6;
	    MKT_MODEL=3;
	    ;;
	FGBM_0)
	    NUM_FILES=3;
	    MKT_MODEL=3;
	    ;;
	FGBL_0)
	    NUM_FILES=1;
	    MKT_MODEL=3;
	    ;;
	FESX_0)
	    NUM_FILES=6;
	    MKT_MODEL=3;
	    ;;
	CGB_0)
	    NUM_FILES=1;
	    MKT_MODEL=2;
	    ;;
	SXF_0)
	    NUM_FILES=1;
	    MKT_MODEL=2;
	    ;;
	BAX_0)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	BAX_1)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	BAX_2)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	BAX_3)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	BAX_4)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	BAX_5)
	    NUM_FILES=1;
	    MKT_MODEL=1;
	    ;;
	*)
	    NUM_FILES=0;
	    MKT_MODEL=1;
	    ;;
    esac
    
# by sharpe
    CHOICEFILE=$HOME/choices/US_MORN_DAY_s_$SHORTCODE.$dstr;
    if [ -e $CHOICEFILE ] ; then
	rm -f $TEMPSTRATFILE ;
	UNIQUEID=$BASE_UNIQUE_SIM_ID;
	for name in `head -n$NUM_FILES $CHOICEFILE | awk '{print $3}'`; 
	do 
	    echo $name;
	    FULLSTRATFILE=~/modelling/strats/*/*/$name
	    if [ -e $FULLSTRATFILE ] ; then
		cat $FULLSTRATFILE | awk '{$8='$UNIQUEID'; print $0}' >> $TEMPSTRATFILE ; 
	    fi
	    UNIQUEID=$(( $UNIQUEID + 1 ));
	done

	if [ $UNIQUEID -gt $BASE_UNIQUE_SIM_ID ]; then
	    ~/basetrade/scripts/run_mult_strats_and_plot.sh $TEMPSTRATFILE $dstr $MKT_MODEL;
	fi
    fi

done