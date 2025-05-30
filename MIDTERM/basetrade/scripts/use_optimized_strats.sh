#!/bin/bash

SHC=CGB_0;
if [ $# -lt 1 ] ; then echo "need shortcode"; exit; fi

SHC=$1; shift;
mkdir -p $HOME/fbmwork/$SHC;
for stratpath in `cat ~/fbmwork/$SHC""_strat_order.txt`; 
do 
    if [ -e $stratpath ] ; then
	stratbase=`basename $stratpath`; 
	stratdirpath=`dirname $stratpath`; 
	optmodelpath=$HOME/fbmwork/$SHC/opt_model_gradalpha10_$stratbase;
	if [ -e $optmodelpath ] ; then 
	    newstratfilepath=$stratdirpath/w_gradalpha10_$stratbase;
	    prevmodelpath=`awk '{print $4}' $stratpath`;
	    prevmodeldir=`dirname $prevmodelpath`;
	    newmodelfilepath=$prevmodeldir/opt_model_gradalpha10_$stratbase;
	    cp $optmodelpath $newmodelfilepath;
	    cat $stratpath | awk -vnmp=$newmodelfilepath '{ $4=nmp ; print }' > $newstratfilepath ;
	fi
    fi
done
