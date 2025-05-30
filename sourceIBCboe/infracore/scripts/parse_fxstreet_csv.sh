#!/bin/bash
#parse CSV file from FXStreet and convert to required  format
#Assumption:- The CSV file contains lines of format: DateTime,Name,Country,Volatility,Actual,Previous,Consensus
if [ $# -lt 2 ] ; then echo "USAGE: <script> <fxstreet_csvfile> <output-csv>"; exit ; fi;

cur_date=`date +%Y%m%d`
#Print this in the required format
echo "\"country\",\"date\",\"name\",\"actual\",\"consensus\",\"previous\",\"volatility\"" > $2
cat $1 | grep -v DateTime | awk -F',' '{yy=substr($1, 7, 4);mm=substr($1, 1, 2);dd=substr($1, 4, 2);tm=substr($1, 12);evt_time=substr($1, 6, 4)""substr($1, 0, 2);print "\""$3"\",\""yy""mm""dd" "tm"\",\""$2"\",\"\",\"\",\"\",\""$4"\""; if($3=="United Kingdom" && $4=="3"){ $3="European Monetary Union"; $4="2"; print "\""$3"\",\""yy""mm""dd" "tm"\",\""$2"\",\"\",\"\",\"\",\""$4"\"";}}' | grep $cur_date >> $2
