#!/bin/bash 

#clear page
touch /tmp/live_fxstreet_economic_events.html
>/tmp/live_fxstreet_economic_events.html


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

#fetch uptodate web page from fxstreet 
wget -O /tmp/live_fxstreet_economic_events.html "http://www.fxstreet.com/fundamental/economic-calendar" 

EXEC=$HOME/infracore_install/bin/parse_fxstreet_economic_events 

$EXEC /tmp/live_fxstreet_economic_events.html output.csv

rm -rf /tmp/live_fxstreet_economic_events.html
