#! /bin/bash

FETCH_EDF_SCRIPT=$HOME/infracore/recon/fetch_edf_files.sh;

startdate=20160501
enddate=20160601

dates=()
for (( date="$startdate"; date != enddate; )); do
  DAY_OF_WEEK=`date -d $date +%w`
  if [ $DAY_OF_WEEK -ge 1 -a $DAY_OF_WEEK -le 5 ] ; then
    dates+=( "$date" )
  fi
  date="$(date --date="$date + 1 days" +'%Y%m%d')"
done

for day in `echo ${dates[*]}`
do
 	echo $day
 	EDF_RAW_FILE="/apps/data/EDFTrades/EDFFiles/PFDFST4_$day.CSV";
	if [ ! -f $EDF_RAW_FILE ] ; then
	  	echo "fetching edf files for $day" 
		$FETCH_EDF_SCRIPT $day 
	fi
	if [ ! -f $EDF_RAW_FILE ] ; then
	   echo "edf files for $day not fetched."
	fi
	
	./recon_commission.sh $day
done