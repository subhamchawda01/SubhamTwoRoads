#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 SHORTCODE LIST_OF_SHORTCODE -1 PORT LIST_OF_PORTFOLIOS -1 DATE PRICE_TYPE UNIX_START_TIME UNIX_END_TIME TIME_INTERVAL(USEC/-1 All update) OriginalPrice?(0/1)"
EXAMP1="$0 SHORTCODE FESX_0 FGBM_0 -1 PORT ULEQUI -1 20130108 MktSizeWPrice 1200 1300 -1 0" 

if [ $# -lt 8 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit ;
fi

PRICE_DATA_EXEC=$HOME/basetrade_install/bin/generate_price_data 

TEMP_OUTPUT_FILE=/tmp/price_data.dat 
TEMP_CMD_FILE=/tmp/price_data_cmd.dat
LIST_OF_SHORTCODE_FILE=/tmp/list_of_shortcode.txt
LIST_OF_PORT_FILE=/tmp/list_of_port.txt

rm -rf /tmp/*PLOT_PRICE_DATA*

echo "$PRICE_DATA_EXEC $*" ;

$PRICE_DATA_EXEC $* > $TEMP_OUTPUT_FILE ;

echo $* > $TEMP_CMD_FILE ; 

cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep "SHORTCODE" | sed 's/SHORTCODE //g' > $LIST_OF_SHORTCODE_FILE ;
cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep "PORT" | sed 's/PORT//g' > $LIST_OF_PORT_FILE ; 

gnuplot_command='
set xdata time;                    
set timefmt "%s";                  
set grid ;
set format x "%H:%M";
plot '

extra_cmd=''

for shortcode in `cat $LIST_OF_SHORTCODE_FILE`
do

  FILE=/tmp/$shortcode"_PLOT_PRICE_DATA";

  grep "$shortcode" $TEMP_OUTPUT_FILE > $FILE ;

  extra_cmd="$extra_cmd, '$FILE' using 1:3 with lines title '$shortcode'"

done 

for port in `cat $LIST_OF_PORT_FILE`
do

  FILE=/tmp/$port"_PLOT_PRICE_DATA" ;

  grep "$port" $TEMP_OUTPUT_FILE > /tmp/$port"_PLOT_PRICE_DATA" ;

  extra_cmd="$extra_cmd, '$FILE' using 1:3 with lines title '$port'"

done 

echo "$gnuplot_command  ${extra_cmd:1};"  | gnuplot -persist


rm -f $TEMP_OUTPUT_FILE ;
rm -f $TEMP_CMD_FILE;
rm -f $LIST_OF_SHORTCODE_FILE;
rm -f $LIST_OF_PORT_FILE ;
rm -f /tmp/*PLOT_PRICE_DATA*
