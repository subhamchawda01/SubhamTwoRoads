#!/bin/bash

if [ $# -lt 2 ] ; then echo "orig_strat new_model"; exit; fi

stratpath=$1;
optmodelpath=$2;

if [ -e $stratpath ] ; then
    if [ -e $optmodelpath ] ; then 
	stratbase=`basename $stratpath`; 
	stratdirpath=`dirname $stratpath`; 
	install_stratdirpath=`echo $stratdirpath | sed -e 's#pruned_strategies#strats#g' `;
#	install_stratdirpath=`echo $stratdirpath | replace pruned_strategies strats`;
	optmodelbase=`basename $optmodelpath`;
	newstratfilepath=$install_stratdirpath/w_opt_$optmodelbase;
	prevmodelpath=`awk '{print $4}' $stratpath`;
	prevmodeldir=`dirname $prevmodelpath`;
	newmodelfilepath=$prevmodeldir/w_opt_$optmodelbase;
	cp $optmodelpath $newmodelfilepath;
	cat $stratpath | awk -vnmp=$newmodelfilepath '{ $4=nmp ; print }' > $newstratfilepath ;
    fi
else
    echo $stratpath" missing!"
fi
