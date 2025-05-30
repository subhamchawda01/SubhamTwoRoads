#!/bin/bash
USAGE="$0 SHORTCODE DATE START_TIME END_TIME"

if [ $# -lt 4 ] ;
then
    echo $USAGE;
        exit;
        fi

SHC=$1;shift;
DATE=$1;shift;
STARTHHMM=$1;shift;
ENDHHMM=$1;shift;

#echo $SHC $DATE $STARTHHMM $ENDHHMM
DATAGEN_EXEC=$HOME/basetrade_install/bin/datagen
UNIX_CONVERTER=$HOME/infracore/scripts/get_unix_time.pl
ILISTDIR=/spare/local/temp
rm -rf $ILISTDIR/*plot_l1_size*
ILISTN=$ILISTDIR/ilist_plot_l1_size_"$SHC"
echo -e "MODELINIT DEPBASE $SHC MktSizeWPrice MktSizeWPrice\nMODELMATH LINEAR CHANGE\nINDICATORSTART\nINDICATOR 1.00 L1SizeTrend $SHC 1 MidPrice\nINDICATOREND" > $ILISTN
#START_TIME=`$HOME/infracore/scripts/get_unix_time.pl $DATE UTC_0000`
#echo $START_TIME
$HOME/basetrade_install/bin/datagen $ILISTN $DATE $STARTHHMM $ENDHHMM 2222 $ILISTDIR/plot_l1_size_datagen_out 1000 0 0 1 

cat $ILISTDIR/plot_l1_size_datagen_out | awk '{$1=$1/1000 ; print $0}' > $ILISTDIR/plot_l1_size_datagen_out_modified

gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set grid ;
set format x "%H:%M";
plot '

echo "$gnuplot_command '$ILISTDIR/plot_l1_size_datagen_out_modified' using 1:5 with lines title '$SHC';" | gnuplot -persist


rm -f $ILISTDIR/plot_l1_size_datagen_out
rm -f $ILISTDIR/plot_l1_size_datagen_out_modified
rm -f $ILISTDIR/*plot_l1_size*

