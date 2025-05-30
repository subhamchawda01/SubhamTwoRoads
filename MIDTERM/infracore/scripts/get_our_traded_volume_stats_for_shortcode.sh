#!/bin/bash

#TODO currently unused later on can specify time range etc.
USAGE1="$0 SHORTCODE YYYYMM SUMMARY/PLOT"
EXAMP1="$0 FESX 201203 PLOT"

if [ $# -ne 3 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SHORTCODE=$1
YYYYMM=$2
OUTFORMAT=$3

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


VOL_DATA_DIR=/NAS1/data/MFGlobalTrades/ProductPnl/${YYYYMM:0:4}/${YYYYMM:4:2}
PNL_FILE=pnl.csv

TEMP_VOL_FILE=/tmp/our_traded_volumes_temp.txt
>$TEMP_VOL_FILE

BAX_TEMP_VOL_FILE=/tmp/our_traded_volumes_bax_temp.txt
>$BAX_TEMP_VOL_FILE

TOTALDAYS=0

for dir in `ls $VOL_DATA_DIR`
do

  if [ -f $VOL_DATA_DIR/$dir/$PNL_FILE ]
  then

     LINE_COUNT=`grep $SHORTCODE $VOL_DATA_DIR/$dir/$PNL_FILE | wc -l` ;

     if [ $LINE_COUNT -gt 0 ]
     then

       ((TOTALDAYS++)) ;

       if [ $SHORTCODE == "BAX" ] || [ $SHORTCODE == "LFI" ] || [ $SHORTCODE == "LFL" ] || [ $SHORTCODE == "YFEBM" ] || [ $SHORTCODE == "XFC" ] || [ $SHORTCODE == "XFRC" ]
       then 

         echo `grep $SHORTCODE $VOL_DATA_DIR/$dir/$PNL_FILE | tr ',' ' ' |awk '{if($4 > 0) {print $4} else {print "0"}}'` >> $BAX_TEMP_VOL_FILE ;
 
       else 

         echo $YYYYMM$dir `grep $SHORTCODE $VOL_DATA_DIR/$dir/$PNL_FILE | tr ',' ' ' |awk '{if($4 > 0) {print $4} else {print "0"}}'` >> $TEMP_VOL_FILE ;

       fi ;

     fi ;
   
  fi

done

if [ $SHORTCODE == "BAX" ] || [ $SHORTCODE == "LFI" ] || [ $SHORTCODE == "LFL" ] || [ $SHORTCODE == "YFEBM" ] || [ $SHORTCODE == "XFC" ] || [ $SHORTCODE == "XFRC" ]
then 

  for i in `cat $BAX_TEMP_VOL_FILE | tr ' ' '+' | bc`
  do

    echo $YYYYMM$dir $i >> $TEMP_VOL_FILE ;
  
  done 

fi


if [ $OUTFORMAT = "PLOT" ]
then 

   GP_CMD='set title "Our Traded Volumes"; set xlabel "Trading-Date"; set ylabel "Volumes"; set xdata time; set timefmt "%Y%m%d"; plot "'$TEMP_VOL_FILE'" using 1:2 with lines'; 
   echo $GP_CMD | gnuplot -persist  

else 

  if [ $OUTFORMAT = "SUMMARY" ]
  then

    TotalVol=`cat $TEMP_VOL_FILE | awk '{sum += $2; print sum}' | tail -1`
    MaxVol=`sort -nr -k 2 $TEMP_VOL_FILE | head -1 | awk '{print $2 " On " $1}'`
    MinVol=`sort -nr -k 2 $TEMP_VOL_FILE | tail -1 | awk '{print $2 " On " $1}'`
 
    echo $YYYYMM $SHORTCODE " -> " "Total : "$TotalVol "TradingDays : "$TOTALDAYS "Max : "$MaxVol "Min : "$MinVol "Avg : "`echo $TotalVol $TOTALDAYS | awk '{print $1/$2}'` 

  fi
 
fi

rm -rf $TEMP_VOL_FILE
rm -rf $BAX_TEMP_VOL_FILE
