#!/bin/bash

if [ $# -lt 3 ] ; then echo "USAGE: " $0 "configfile paramname value"; exit 0; fi;

config=$1; 
param=$2; 
value=$3; 

uid=`date +%N`;
tmpfile=tmp_"$uid";
cat $config | awk -vpf=$param -vpv=$value '{if($2==pf){$3=pv;} {print $_;}}' > $tmpfile;
mv $tmpfile $config;
