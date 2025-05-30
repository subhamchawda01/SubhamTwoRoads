#!/bin/bash

if [ $# -lt 1 ] ; then echo "USAGE: <script> <fxstreet_csvfile>"; exit ; fi;

cat $1  | replace " " "_" "," " " | awk '{cmd="~/infracore/scripts/get_date_time_from_eco_fmt.sh "$1; cmd | getline dt_tm; close(cmd);  cmd="~/infracore/scripts/get_currency_from_country.sh "$3; cmd | getline curr; close(cmd);  if(curr!="EXT"){print "~/infracore_install/bin/get_event_line \""$2"\"", curr, $4, dt_tm;} }' | sh
