#!/bin/bash

USAGE="$0 SHC STRATFILENAME HCOUNT ";
if [ $# -lt 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

SHC=$1; shift;
STRATFILENAME=$1; shift;
HCOUNT=$1; shift

if [ -d $HOME/ec2_staged_globalresults ] ; then 
    grep -hw $STRATFILENAME $HOME/ec2_staged_globalresults/$SHC/201?/*/??/r*.txt | awk '{ $1=""; print; }' | sort -g | uniq | tail -n$HCOUNT
else
    if [ -d /NAS1/ec2_staged_globalresults ] ; then 
	grep -hw $STRATFILENAME /NAS1/ec2_staged_globalresults/$SHC/201?/*/??/r*.txt | awk '{ $1=""; print; }' | sort -g | uniq | tail -n$HCOUNT
    fi
fi
