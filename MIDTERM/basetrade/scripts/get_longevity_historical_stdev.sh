#!/bin/bash

USAGE="$0 SHORTCODE STARTHHMM ENDHHMM";
if [ $# -lt 3 ] ;
then
    echo $USAGE;
    exit;
fi

SHC=$1; shift;
STARTHHMM=$1; shift;
ENDHHMM=$1; shift;
#echo $SHC $STARTHHMM $ENDHHMM;

NUM_DAYS=200;
YESTERDAY=$($HOME/basetrade_install/bin/calc_prev_week_day $(date  +%Y%m%d));
END_DATE=$YESTERDAY;
START_DATE=$($HOME/basetrade_install/bin/calc_prev_week_day $YESTERDAY $NUM_DAYS);
LONGEVITY_FILE="/spare/local/tradeinfo/PCAInfo/shortcode_stdev_longevity.txt"

if grep -wq $SHC $LONGEVITY_FILE ; then
  echo "$SHC already exist in $LONGEVITY_FILE.. exiting..";
  exit 0;
fi

if [ ! -d $HOME/longevity ] ; then
  mkdir $HOME/longevity;
fi

ILISTDIR=$HOME/longevity/ilist_"$SHC"_dir
if [ ! -d $ILISTDIR ] ; then
  mkdir $ILISTDIR;
fi

ILISTN=$ILISTDIR/ilist_"$SHC"
echo -e "MODELINIT DEPBASE $SHC MktSizeWPrice MktSizeWPrice\nMODELMATH LINEAR CHANGE\nINDICATORSTART\nINDICATOR 1.00 SlowStdevCalculator $SHC 300.0 0.00\nINDICATOREND" > $ILISTN

#$HOME/basetrade_install/ModelScripts/generate_timed_indicator_data.pl $ILISTN $START_DATE $END_DATE $STARTHHMM $ENDHHMM 40000 0 0 $ILISTDIR"/gendata" 1>/dev/null 2>&1; 
$HOME/basetrade/ModelScripts/get_gendata.R $SHC $ILISTN $END_DATE $NUM_DAYS $STARTHHMM $ENDHHMM 40000 0 0 0 $ILISTDIR"/" 1>/dev/null 2>&1;

if [ -e $ILISTDIR/catted_gendata_outfile ] ; then 
  AVG_STDEV=`cat $ILISTDIR/catted_gendata_outfile | awk '{SUM += \$2;} END { print SUM / NR; }'`;
fi

if [[ $AVG_STDEV ]]; then 
  echo SHORTCODE_STDEV_LONGEVITY $SHC $AVG_STDEV >> $LONGEVITY_FILE;
fi
#echo SHORTCODE_STDEV_LONGEVITY $SHC `$HOME/basetrade/scripts/get_daywise_avg.R $ILISTDIR/catted_gendata_outfile $ILISTDIR/daywise_stdev.txt`
