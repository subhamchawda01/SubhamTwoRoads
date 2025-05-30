#!/bin/bash -e

USAGE1="$0 YYYYMMDD ";
USAGE2="$0 YYYYMMDD CMD[COPYDATA/COPYALL]";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE1;
    echo $USAGE2;
    exit;
fi


EXEC=$HOME/infracore_install/bindebug/exchange_symbol_vomapper 
COPY_EXEC=$HOME/LiveExec/bin/copy_volsym.pl
INPUT_SYM_FILE=$HOME/infracore_install/files/volume_based_symbols_to_eval.txt



YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

#first generate the .orig.txt & .basecodes.txt  files
$HOME/infracore_install/scripts/all_volumes_of_day_all_exchanges.sh $YYYYMMDD 
$EXEC $INPUT_SYM_FILE $YYYYMMDD 

if [ $# -eq 2 ];
then 
    CMD=$2;
case $CMD in
    copydata|COPYDATA)
	ARG="/spare/local/VolumeBasedSymbol/VOSymbol_"$YYYYMMDD".txt"
	echo "copying to all machines  $ARG"
	echo $COPY_VOLSYM_EXEC
	`perl -w "$COPY_VOLSYM_EXEC" $ARG`
	;;

    copyall|COPYALL)
	echo $COPY_VOLSYM_EXEC
	DIR=/spare/local/VolumeBasedSymbol	
	for i in $(ls $DIR); do
	    item=$DIR/$i
	    if [[ $item =~ VOSymbol_* ]]
		then 
		echo $item
		`perl -w "$COPY_VOLSYM_EXEC" $item`
	    fi
	done
	;;

esac


fi