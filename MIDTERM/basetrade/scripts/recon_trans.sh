#!/bin/bash

USAGE1="$0 DATE"
EXAMP1="$0 YESTERDAY"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1;

if [ "$YYYYMMDD" == "YESTERDAY" ]
then 

    YYYYMMDD=`cat /tmp/YESTERDAY_DATE`

fi 

if [ "$YYYYMMDD" == "TODAY" ]
then 

    YYYYMMDD=`date +"%Y%m%d"` ;

fi 


AUDIT_LOG_GENERATE_EXEC=$HOME/basetrade_install/bin/generate_newedge_audit_log 
EXCH_SYMBOL_EXEC=$HOME/basetrade_install/bin/get_exchange_symbol 

SHORTCODE_FILE_LIST=/spare/local/files/newedge_audit_config.cfg
TEMP_FILE_FOR_TRADES=/tmp/newedge_trades_temp.txt 

DESTINATION_SUDIT_LOG_DIR=/apps/data/MFGlobalTrades/AUDIT 

AUDIT_LOG_FILE=$YYYYMMDD".AUDIT.log" ;

for product in `cat $SHORTCODE_FILE_LIST | awk -F"~" '{print $1}'`
do 

    yyyy=`echo ${YYYYMMDD:0:4}` ;
    mm=`echo ${YYYYMMDD:4:2}` ;
    dd=`echo ${YYYYMMDD:6:2}` ;

    this_symbol_=`$EXCH_SYMBOL_EXEC $product $YYYYMMDD` ;
    this_exch_symbol_=`echo $this_symbol_ | tr ' ' '~' | sed 's/~/~~/g'`;

    #simple hack to deal with XFRC, just 1 space, change later 
    if [[ "$this_exch_symbol_" == *XFRC* ]]
    then

        this_exch_symbol_=`echo $this_symbol_ | sed 's/ /~/g'`;
    fi

    exchidfile=`grep $product $SHORTCODE_FILE_LIST | awk -F"~" '{print $2}'`"."$YYYYMMDD".dat";
    echo "grep $product $SHORTCODE_FILE_LIST | awk -F~ '{print \$2 \".$YYYYMMDD.dat\"}'";
    tradesfile=`grep $product $SHORTCODE_FILE_LIST | awk -F"~" '{print $3}'`"/"$yyyy"/"$mm"/"$dd"/trades."$YYYYMMDD;
    echo "grep $product $SHORTCODE_FILE_LIST | awk -F~ '{ print \$3 \"/$yyyy/$mm/$dd/trades.$YYYYMMDD\" }'";
    data=`grep $product $SHORTCODE_FILE_LIST | awk -F"~" '{print $4}'` ;
    echo "grep $product $SHORTCODE_FILE_LIST | awk -F~ '{print \$4}'" ;
    output_file=`grep $product $SHORTCODE_FILE_LIST | awk -F"~" '{print $5}'`""$YYYYMMDD;
    echo "grep $product $SHORTCODE_FILE_LIST | awk -F~ '{print \$5\"$YYYYMMDD\" }'";

    this_dest_dir=$DESTINATION_SUDIT_LOG_DIR"/"$yyyy"/"$mm"/"$dd ;

    mkdir -p $this_dest_dir ;

    cat $tradesfile | tr ' ' '~' | grep $this_exch_symbol_ | tr '\001' ' ' | awk '{print $5 " " $4 " " $3 " " $2}' > $TEMP_FILE_FOR_TRADES ;
    echo "cat $tradesfile | tr ' ' '~' | grep $this_exch_symbol_ | tr '\001' ' ' | awk '{print \$5 \" \" \$4 \" \" \$3 \" \" \$2}' > $TEMP_FILE_FOR_TRADES ";

    #$AUDIT_LOG_GENERATE_EXEC $product $YYYYMMDD $exchidfile $TEMP_FILE_FOR_TRADES $data >> $this_dest_dir/$output_file ;
    echo "/$AUDIT_LOG_GENERATE_EXEC /$product /$YYYYMMDD /$exchidfile /$TEMP_FILE_FOR_TRADES /$data >> $this_dest_dir/$output_file ";

done 

#############
