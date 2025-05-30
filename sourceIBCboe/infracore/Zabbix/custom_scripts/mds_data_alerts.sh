#!/bin/bash

#in case we are getting a filename 
exch=`echo $1 | awk -F"/" '{print $NF}'` ;

if [ ! -s $1 ] ; then 
  echo "DATA LOGGER FOR "$exch" HAS BEEN RESTARTED" ;
else 
  tail -1 $1 ;
fi 
