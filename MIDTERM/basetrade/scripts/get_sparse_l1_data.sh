#!/bin/bash

if [ $# -lt 2 ] ; then echo "USAGE: $0 <shc> <date> [timeout_msecs=1000] [out_mode(0|1)=0]" ; exit 1; fi ;

shc=$1; shift;
dt=$1; shift;
timeout=1000;
if [ $# -ge 1 ] ; then timeout=$1; shift; fi;
out_mode=0;
if [ $# -ge 1 ] ; then out_mode=$1; shift; fi;


sec=`$HOME/basetrade_install/bin/get_exchange_symbol $shc $dt`; 
exch=`$HOME/basetrade_install/bin/get_exch_from_shortcode $shc $dt`;
feed="";
if [ $exch == "BMF" ] ; then
  feed="NTP" ;
fi;

cmd="$HOME/basetrade_install/bin/mkt_trade_logger SIM $shc $dt 1 0 DEF DEF $timeout $feed";
if [ $out_mode -eq 0 ] ; then 
  $cmd 2>/dev/null | replace "[" "" "]" "" | awk -vs=$sec '{printf "%s %s ", $1, s; for(i=5;i>=0;i--){printf "%s ", $(NF-i);} print ""; }'
else
  $cmd 2>/dev/null | replace "[" "" "]" ""
fi
