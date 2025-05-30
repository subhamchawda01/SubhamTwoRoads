#!/usr/bin/env bash

USAGE="$0 SHC STRATFILENAME ";
if [ $# -lt 2 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHC=$1; shift;
STRATFILENAME=$1; shift;

if [ -d $HOME/ec2_globalresults ] ; then 
    for result_filename in $HOME/ec2_globalresults/$SHC/201?/??/??/r*.txt ; do
	sed -i '/'$STRATFILENAME'/d' $result_filename ;
    done
fi
