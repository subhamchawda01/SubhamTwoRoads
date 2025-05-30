#!/bin/bash 

USAGE1="$0 MMDDYYYY "

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    exit;
fi

MMDDYYYY=$1;

#clear page
touch /tmp/live_tradingeconomics_economic_events.html
>/tmp/live_tradingeconomics_economic_events.html

#fetch uptodate web page from fxstreet 
wget -O /tmp/live_tradingeconomics_economic_events.html "http://www.forexpros.com/economic-calendar/"

cat /tmp/live_tradingeconomics_economic_events.html | sed 's/tr>/tr>\n/g' | grep -v "Holiday" > /tmp/live_forexpros_economic_events.html

EXEC=$HOME/prod/live_execs/parse_investingdotcom_economic_events
OUTPUT_FILE="/apps/data/InvestingDotComFiles/investing_event_$MMDDYYYY.csv";
LOG_FILE="/apps/data/InvestingDotComFiles/fetch_investing_log";

$EXEC /tmp/live_forexpros_economic_events.html $OUTPUT_FILE >$LOG_FILE 2>$LOG_FILE

rm -rf /tmp/live_tradingeconomics_economic_events.html 
rm -rf /tmp/live_forexpros_economic_events.html
