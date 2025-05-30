#!/bin/bash

USAGE="<script> template_config [ output_queue=runfbmfs_queue ] [ num_days=60 ] [shc_list=/spare/local/tradeinfo/curr_trade_prod_list.txt] [fbmfsworkdir=$HOME/modelling/fbmfswork] [results_dir=/NAS1/ec2_globalresults/]";
if [ $# -le 0 ]; then echo $USAGE; exit; fi;

template_config=$1; shift;
output_queue="runfbmfs_queue";
num_days=60;
shc_list=`cat /spare/local/tradeinfo/curr_trade_prod_list.txt`;
fbmfsworkdir="$HOME/modelling/fbmfswork";
results_dir="/NAS1/ec2_globalresults/";
if [ `hostname` == "sdv-crt-srv11.dvcap.local" ]; then results_dir="$HOME/ec2_globalresults/"; fi;
if [ $# -ge 1 ] ; then output_queue=$1; shift; fi;
if [ $# -ge 1 ] ; then num_days=$1; shift; fi;
if [ $# -ge 1 ] ; then shc_list=`cat $1`; shift; fi;
if [ $# -ge 1 ] ; then fbmfsworkdir=$1; shift; fi;
if [ $# -ge 1 ] ; then results_dir=$1; shift; fi;


dt=`date +%Y%m%d`;
startdate=`$HOME/basetrade_install/bin/calc_prev_week_day $dt $num_days`;
enddate=`$HOME/basetrade_install/bin/calc_prev_week_day $dt 1`;
bkp_dir="$HOME/fbmfswork_bkp"

rm -rf $bkp_dir;
mkdir -p $bkp_dir;
rm -f $output_queue;

for shc in $shc_list; do
    i="$HOME/modelling/strats/$shc";
    if [ -d $i ]; then
	cp -r --parents $fbmfsworkdir/$shc/fstrat* $bkp_dir; rm -rf $fbmfsworkdir/$shc/fstrat*;
	cp -r --parents $fbmfsworkdir/$shc/fconfig* $bkp_dir; rm -rf $fbmfsworkdir/$shc/fconfig*;
	for j in "$i"/*; do
	    tp=`basename $j`
	    mkdir -p $fbmfsworkdir
	    tmpstratfile="tmp_strat.txt"
	    rm -f $tmpstratfile
	    $HOME/basetrade_install/bin/summarize_strategy_results $shc $j/ $results_dir $startdate $enddate INVALIDFILE kCNAPnlAverage | awk -vdir="$j" '{if($3>0){strat=dir"/"$2; "grep -e EventPriceBasedAggressiveTrading -e EventDirectionalAggressiveTrading "strat" | wc -l " | getline g; if(g==0){print $2}}}' | grep -v 'w_opt_\|w_o_\|EARTH\|w_r_\|w_reg_\|RANDOMFOREST\|BOOSTING\|regime_\|regm'| head -n15 > $tmpstratfile
	    if [ -s $tmpstratfile ]
		then  
		    mkdir -p $fbmfsworkdir/$shc
		    stratfile="$fbmfsworkdir/$shc/fstrat_"$shc"_$tp.txt"
		    cp $tmpstratfile $stratfile
		    if [ `wc -l $stratfile | awk '{print $1}'` -gt 5 ]
			then 
			    split -l5 $stratfile $stratfile
			    rm $stratfile  
		    fi
	    fi 
	done  
    fi

    for sfile in `ls $fbmfsworkdir/$shc/fstrat_*` ; do 
    	configfile=`echo $sfile | replace "fstrat" "fconfig_v1"` 
    	cp $template_config $configfile
    	echo $sfile >> $configfile
    	echo "/home/dvctrader/basetrade_install/ModelScripts/run_find_best_model_distributed.pl $configfile" >> $output_queue
    done
done


