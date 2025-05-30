#!/bin/bash

USAGE="$0 DC_TRADES_FILE_NAME ORS_TRADES_FILE_NAME";
USAGE_DESC=" convert DC_TRADES_FILE_NAME to ors format and saves it to ORS_TRADES_FILE_NAME";

if [ $# -ne 2 ];
then
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

DC_FILE=$1
ORS_FILE=$2

cat  $DC_FILE | awk -F, '{if( $19 == "Ask") { print $12, 1, $29, $28, $24 } else { print $12, 0, $29, $28, $24}}' | tail -n +2 | sed 's/ /\x01/g' > $ORS_FILE
