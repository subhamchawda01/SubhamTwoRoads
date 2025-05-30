#!/bin/bash

if [ $# -lt 2 ] ; then echo "USAGE: " $0 "prod n_uts"; exit 0; fi;

prod=$1;
n_uts=$2; 

for param in `ls ~/modelling/stratwork/$prod/para*` ; 
do 
    echo $param;
    o_uts=`grep UNIT_TRADE_SIZE $param | awk '{print $3}'` ;
    if [ $o_uts > 0 ] ; then
	o_otl=`grep MAX_OPENTRADE_LOSS $param | awk '{print $3}'` ;
	let n_otl=$(( $((o_otl/o_uts)) * n_uts )) ;
	echo $n_otl" "$n_uts ;
	sed -i "s/UNIT_TRADE_SIZE $o_uts/UNIT_TRADE_SIZE $n_uts/g" $param ;
	sed -i "s/MAX_OPENTRADE_LOSS $o_otl/MAX_OPENTRADE_LOSS $n_otl/g" $param ;
    fi ;
done

for param in `ls ~/modelling/params/$prod/para*` ; 
do 
    echo $param;
    o_uts=`grep UNIT_TRADE_SIZE $param | awk '{print $3}'` ;
    if [ $o_uts > 0 ] ; then
	o_otl=`grep MAX_OPENTRADE_LOSS $param | awk '{print $3}'` ;
	let n_otl=$(( $((o_otl/o_uts)) * n_uts )) ;
	echo $n_otl" "$n_uts ;
	sed -i "s/UNIT_TRADE_SIZE $o_uts/UNIT_TRADE_SIZE $n_uts/g" $param ;
	sed -i "s/MAX_OPENTRADE_LOSS $o_otl/MAX_OPENTRADE_LOSS $n_otl/g" $param ;
    fi ;
done



# n_uts=2; for param in `ls ~/modelling/stratwork/CGB_0/param_CGB_0_US_PB_mnl1` ; do o_uts=`grep UNIT_TRADE_SIZE $param | awk '{print $3}'` ;  o_otl=`grep OPENTRADE $param | awk '{print $3}'` ; n_otl=$(($((o_otl/o_uts))*n_uts)); echo $n_otl" "$n_uts; sed -i "s/UNIT_TRADE_SIZE $o_uts/UNIT_TRADE_SIZE $n_uts/g" $param ; sed -i "s/MAX_OPENTRADE_LOSS $o_otl/MAX_OPENTRADE_LOSS $n_otl/g" $param ; done
