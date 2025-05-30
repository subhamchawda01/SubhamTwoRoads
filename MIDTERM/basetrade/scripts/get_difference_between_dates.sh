#!/bin/bash

USAGE="$0 DAY_1 DAY_2";
if [ $# -lt 2 ];
then
	echo $USAGE;
	exit;
fi

START_DAY=$1; shift;
END_DAY=$1; shift;

counter=0;
dt=$END_DAY;

while [ $dt -gt $START_DAY ];do
	counter=$((counter+1));
	dt=`/home/pengine/prod/live_execs/calc_prev_week_day $dt`;
done

echo $counter;
