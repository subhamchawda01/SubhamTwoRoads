#!/bin/bash

USAGE="<script> [ output_queue=runfbmfsregime_queue ] [ num_days=60 ] [shc_list=/spare/local/tradeinfo/curr_trade_prod_list.txt] [fbmfsworkdir=$HOME/modelling/fbmfswork] [results_dir=/NAS1/ec2_globalresults/]";
#if [ $# -le 0 ]; then echo $USAGE; exit; fi;

#template_config=$1; shift;
output_queue="runfbmfsregime_queue";
num_days=60;
shc_list=`cat /spare/local/tradeinfo/curr_trade_prod_list_regime.txt`;
fbmfsworkdir="$HOME/modelling/fbmfswork";
results_dir="/NAS1/ec2_globalresults/";
if [ `hostname` == "sdv-crt-srv11.dvcap.local" ]; then results_dir="$HOME/ec2_globalresults/"; fi;
if [ $# -ge 1 ] ; then output_queue=$1; shift; fi;
if [ $# -ge 1 ] ; then num_days=$1; shift; fi;
if [ $# -ge 1 ] ; then shc_list=`cat $1`; shift; fi;
if [ $# -ge 1 ] ; then fbmfsworkdir=$1; shift; fi;
if [ $# -ge 1 ] ; then results_dir=$1; shift; fi;


dt=`date +%Y%m%d`;
startdate=`$HOME/LiveExec/bin/calc_prev_day $dt $num_days`;
enddate=`$HOME/LiveExec/bin/calc_prev_day $dt 1`;

mkdir -p fbmfswork_regime_bkp;
rm -f $output_queue;

for shc in $shc_list; do
    i="$HOME/modelling/strats/$shc";
    if [ -d $i ]; then
	cp -r --parents $fbmfsworkdir/$shc/fregimestrat* fbmfswork_regime_bkp; rm $fbmfsworkdir/$shc/fregimestrat*;
	for j in "$i"/*; do
	    tp=`basename $j`
	    mkdir -p $fbmfsworkdir
	    tmpstratfile="tmp_strat.txt"
	    rm -f $tmpstratfile
	    $HOME/basetrade_install/bin/summarize_strategy_results $shc $j/ $results_dir $startdate $enddate INVALIDFILE kCNAPnlAverage | awk -vdir="$j" '{if($3>0){strat=dir"/"$2; "grep -e EventPriceBasedAggressiveTrading -e EventDirectionalAggressiveTrading "strat" | wc -l " | getline g; if(g==0){print $2}}}' | grep -v 'w_reg\|EARTH\|SIGLR\|RANDOMFOREST'| head -n15 > $tmpstratfile
	    if [ -s $tmpstratfile ]
		then  
		    mkdir -p $fbmfsworkdir/$shc
		    stratfile="$fbmfsworkdir/$shc/fregimestrat_"$shc"_$tp.txt"
		    cp $tmpstratfile $stratfile
	    fi 
	done  
    fi
done

for i in $fbmfsworkdir/*/fregimeconfig_* ; do 
    echo "/home/dvctrader/basetrade_install/ModelScripts/run_find_best_model.pl $i" >> $output_queue
done

