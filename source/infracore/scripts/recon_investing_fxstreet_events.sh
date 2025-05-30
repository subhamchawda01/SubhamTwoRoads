#! /bin/bash

USAGE="$0 date(mmddyyyy)"

if [ $# -ne 1 ] ; 
then 
    echo -e $USAGE;
    exit;
fi
MMDDYYYY=$1;
##map to for all the 3 degree volatile events##
declare -A investing_to_fxstreet_events ;

investing_filename="/apps/data/InvestingDotComFiles/investing_event_$MMDDYYYY.csv";
fxstreet_filename="/home/pengine/master/infracore/SysInfo/FXStreetEcoReports/fxstreet_$MMDDYYYY_$MMDDYYYY.csv";

##====keep ADDING mapping for events into this map===##
##====replace spaces by '~', you can use: echo "Core CPI (MoM)  (May)" | tr ' ' '~' ====##
investing_to_fxstreet_events["Core~CPI~(YoY)~~(May)"]="Bank~of~Canada~Consumer~Price~Index~Core~(MoM)";
investing_to_fxstreet_events["Core~CPI~(MoM)~~(May)"]="Bank~of~Canada~Consumer~Price~Index~Core~(YoY)";
investing_to_fxstreet_events["House~Prices~(YoY)~~(May)"]="Consumer~Price~Index~(YoY)";

##===============================================maps ends=================================##

for investing_event in `echo ${!investing_to_fxstreet_events[*]}` ;
do
	fxstreet_event=`echo ${investing_to_fxstreet_events[$investing_event]} | tr '~' ' '`;
	investing_event=`echo $investing_event  | tr '~' ' '`;
	
	echo "events: $fxstreet_event $investing_event";
	investing_time=`grep "$investing_event" "$investing_filename" | tr -d '"' | \
		awk -F"," '{hhmmss=substr($2,10); hh=substr(hhmmss,1,2)+4; mmss=substr(hhmmss,3);print hh""mmss}'`;
	fxstreet_time=`grep "$fxstreet_event" "$fxstreet_filename" | tr -d '"' | awk -F"," '{print substr($2,10)}'`;
	
	echo "time: $investing_time $fxstreet_time";
	if [[ ! -z $investing_time && ! -z $fxstreet_time ]] ; then
		if [ "$investing_time" != "$fxstreet_time" ] ; then
			echo "Discrepancy in timings on both websites";
		else
			echo "Events match";
		fi
	elif [[ -z $investing_time && ! -z $fxstreet_time ]]; then
		echo "Event $investing_event not found on investing.com";
	elif [[ ! -z $investing_time && -z $fxstreet_time ]]; then
		echo "Event $fxstreet_event not found on fxstreet.com";
	fi	
done