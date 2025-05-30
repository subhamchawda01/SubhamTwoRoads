#!/bin/bash

skip=INVALIDFILE;
ed=`date +%Y%m%d`;
if [ $# -ge 6 ] ; then skip=$6; fi;
if [ $# -ge 7 ] ; then ed=$7; fi;
$HOME/basetrade/ModelScripts/find_optimal_max_loss.pl $1 US $3 $4 $5 $2 $ed $skip | egrep -v '[a-z,A-Z]' | column -t | sed 's/\.[0-9]\+//g' | sed 's/\s\+/|/g' | tr '\n' ' ' ;
