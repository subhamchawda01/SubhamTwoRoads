#!/bin/bash

USAGE="$0 shc strat_name start_date end_date [results_dir=~/ec2_globalresults]"
if [ $# -lt 2 ];
then
	echo $USAGE;
	exit;
fi

SHORTCODE=$1;
STRAT=$2;
START_DAY=$3;
END_DAY=$4;
results_dir="/home/dvctrader/ec2_globalresults";

if [ $# -ge 5 ];
then
	results_dir=$5;
fi

dt=$END_DAY;

while [ $dt -ge $START_DAY ];do
	YYYY=${dt:0:4};
	MM=${dt:4:2};
	DD=${dt:6:2};
	result_file="$results_dir/$SHORTCODE/$YYYY/$MM/$DD/results_database.txt";
	if [ -f $result_file ];
	then
		val=`grep $STRAT $result_file | awk '{print \$3}'`;
		if [ "$val" == "" ];
		then
			echo $dt 0;
		else
			echo $dt $val;
		fi
	else
		echo $dt 0
	fi	
	dt=`~/basetrade_install/bin/calc_prev_week_day $dt`;
done
