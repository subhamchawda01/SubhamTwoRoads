#!/bin/bash

#TODO ADD TIME RANGE
USAGE1="$0 SHORTCODE LIST_OF_SHORTCODE -1 PORT LIST_OF_PORTFOLIOS -1 DATE PRICE_TYPE UNIX_START_TIME UNIX_END_TIME TIME_INTERVAL(USEC/-1 All update) OriginalPrice?(0/1) NUM_DAYS"
EXAMP1="$0 SHORTCODE FESX_0 FGBM_0 -1 PORT ULEQUI -1 20130108 MktSizeWPrice 1200 1300 -1 0 10" 

NUM_DAYS=${@: -1};
#echo $NUM_DAYS;
set -- "${@:1:$(($#-1))}";
#echo $*;

if [ $# -lt 9 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit ;
fi

PRICE_DATA_EXEC=$HOME/basetrade_install/bin/generate_price_data 

TEMP_OUTPUT_FILE=/tmp/price_data.dat 
TEMP_ALLOUTPUT_FILE=/tmp/all_price_data.dat 
TEMP_CMD_FILE=/tmp/price_data_cmd.dat
LIST_OF_SHORTCODE_FILE=/tmp/list_of_shortcode.txt
LIST_OF_PORT_FILE=/tmp/list_of_port.txt

rm -rf /tmp/*PLOT_PRICE_DATA*

echo "$PRICE_DATA_EXEC $*" ;

echo $* > $TEMP_CMD_FILE ;
date=`cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep -v "SHORTCODE\|PORT" | head -n1 | awk '{ print \$1}'`;
stime=`cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep -v "SHORTCODE\|PORT" | head -n1 | awk '{ print \$3}'`;
etime=`cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep -v "SHORTCODE\|PORT" | head -n1 | awk '{ print \$4}'`;
cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep "SHORTCODE" | sed 's/SHORTCODE //g' > $LIST_OF_SHORTCODE_FILE ;
cat $TEMP_CMD_FILE | sed 's/ -1 /\n/g' | grep "PORT" | sed 's/PORT//g' > $LIST_OF_PORT_FILE ; 


TEMP_CMD=$*;

gnuplot_command='
set grid ;'

$PRICE_DATA_EXEC $TEMP_CMD > $TEMP_OUTPUT_FILE ;
line1=`wc -l $TEMP_OUTPUT_FILE | awk '{print \$1}'`;
gnuplot_command="$gnuplot_command set arrow from $line1, graph 0 to $line1, graph 1 nohead;"
#echo $gnuplot_command;
cat $TEMP_OUTPUT_FILE > $TEMP_ALLOUTPUT_FILE;
for i in `seq 2 $NUM_DAYS`; do
    prev_date=`~/basetrade_install/bin/calc_next_week_day $date 1`;
    TEMP_CMD="${TEMP_CMD/$date/$prev_date}";
    echo $TEMP_CMD;
    $PRICE_DATA_EXEC $TEMP_CMD > $TEMP_OUTPUT_FILE ;
    line2=`wc -l $TEMP_OUTPUT_FILE | awk '{print \$1}'`;
    line1=$((line2 + line1));
    gnuplot_command="$gnuplot_command set arrow from $line1, graph 0 to $line1, graph 1 nohead;" 
    cat $TEMP_OUTPUT_FILE >> $TEMP_ALLOUTPUT_FILE;
    date=$prev_date;
done


gnuplot_command="$gnuplot_command plot";
#echo $gnuplot_command;

extra_cmd='';
for shortcode in `cat $LIST_OF_SHORTCODE_FILE`
do
  FILE=/tmp/$shortcode"_PLOT_PRICE_DATA";
  grep "$shortcode" $TEMP_ALLOUTPUT_FILE > $FILE ;
  extra_cmd="$extra_cmd, '$FILE' using (column(0)):3 with lines title '$shortcode'"
done 

for port in `cat $LIST_OF_PORT_FILE`
do
  FILE=/tmp/$port"_PLOT_PRICE_DATA" ;
  grep "$port" $TEMP_ALLOUTPUT_FILE > /tmp/$port"_PLOT_PRICE_DATA" ;
  extra_cmd="$extra_cmd, '$FILE' using (column(0)):3 with lines title '$port'"
done 

#extra_cmd="$extra_cmd, set parametric [y=1:100]100,y ";
echo "$gnuplot_command  ${extra_cmd:1};"  | gnuplot -persist


rm -f $TEMP_OUTPUT_FILE ;
rm -f $TEMP_ALLOUTPUT_FILE ;
rm -f $TEMP_CMD_FILE;
rm -f $LIST_OF_SHORTCODE_FILE;
rm -f $LIST_OF_PORT_FILE ;
rm -f /tmp/*PLOT_PRICE_DATA*
