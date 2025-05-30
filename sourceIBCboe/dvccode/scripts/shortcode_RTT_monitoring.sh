#!/bin/bash

#Init
shc_file=/home/pengine/prod/live_configs/shortcode_for_RTT_monitoring.txt
temp_file=~/trash/temp_RTT.txt
dt=`date -d "yesterday" '+%Y%m%d'`

ors_delay_exec=~/basetrade_install/bin/ors_delay_calc_new
send_slack_exec=/home/pengine/prod/live_execs/send_slack_notification

for entry in `cat $shc_file`
do
	#Check for commented lines
	if [[ "$entry" = \#* ]]
	then
    	continue
	fi

	shc=`echo $entry | awk -F'|' {'print $1'}`
	kPer=`echo $entry | awk -F'|' {'print $2'}`
	thresh=`echo $entry | awk -F'|' {'print $3'}`

	$ors_delay_exec $shc $dt | awk {'print $3'} | sort -n > $temp_file

	#Total no of lines
	x=`wc -l $temp_file | awk {'print $1'}`
	if [ $x -eq 0 ] #Continue if file is empty
	then
		continue
	fi

	#Check for the minimum value less than equal to 0
	var=`head -1 $temp_file`
	var=$(echo $var | awk '{ print $1*1000000}')
	if [ "$var" -le 0 ]
	then
		error_msg="$shc | $dt | RTT_MIN:$var (Threshold 0 breach)"
		$send_slack_exec sim-infra DATA "$error_msg"
	fi

	#Check for Kth percentile values
	kPerFrac="0."$kPer
	RES=$(echo "scale=4; $x*$kPerFrac" | bc)
	RES=`echo $RES | awk -F'.' {'print $1'}`
	var=`head -$RES $temp_file | tail -1`
	var=$(echo $var | awk '{ print $1*1000000}')
	if [ "$var" -lt "$thresh" ]
	then
		error_msg="$shc | $dt | RTT_$kPer.P: $var (Threshold $thresh breach)"
		$send_slack_exec sim-infra DATA "$error_msg"
	fi
done

#CleanUp
if [ -f $temp_file ]
then
	rm $temp_file
fi