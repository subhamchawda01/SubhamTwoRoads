#!/bin/bash

usage () {

  echo "USAGE : $0 <SPREADSHEET>" ;
  exit ;

}

[ -n "$1" -a -s "$1" -a -f "$1" ] || usage ;

TEMP_FILE=/tmp/cfe_processed_csv.csv

ssconvert $1 $TEMP_FILE

cat $TEMP_FILE | awk -F"," '{ if ( $9 == "O" ) { print $1 " 1 " $23 " " $22 " 9999"} else if ( $18 == "O" ) { print $1 " 0 " $23 " " $22 " 9999" } }' | sed 's/VX VX /VX/g' | sed 's/VX VX/VX_SPRD/g'  | sed 's/"//g'| tr ' ' '\001'

rm -rf $TEMP_FILE ;
