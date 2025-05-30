#!/bin/bash
USAGE="$0 YYYYMMDD";
if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

update_dcstatus_remote(){
ssh dvcinfra@10.23.74.51 'bash -s' < /home/pengine/prod/live_scripts/status_update_mysql.sh "$1" "$2" "$3" "$4" "$5"
}

DATA_CONVERTER_EXEC="/home/pengine/prod/live_execs/nse_generic_to_dotex_offline_converter";
CONVERTED_DATA_DIR="/spare/local/MDSlogs/NSE/";

YYYYMMDD=$1
LOG_FILE="/spare/local/logs/alllogs/nse_generic_to_dotex_converter.log.$YYYYMMDD";
> $LOG_FILE

fileExt="";
if [ `ls /spare/local/MDSlogs/GENERIC/NSE*$YYYYMMDD | wc -l` -le 0 ]; then #generic files compressed
	fileExt=".gz";
fi

#marking the nse data converter has been started
update_dcstatus_remote "setStatus" "5" "109" "$YYYYMMDD" "10.23.115.61"

#Dump MDS files (from CombinedShmWriter) before initiating datacopy
/home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 0
sleep 10

#delete converted files older than 2 day
find $CONVERTED_DATA_DIR -type f -mtime +2 -exec rm -f {} \;

echo "EMAIL  Starting Conversion all GENERIC at `date`" >> $LOG_FILE 2>>$LOG_FILE
count=40
total=40
for filename in `ls /spare/local/MDSlogs/GENERIC/NSE*$YYYYMMDD$fileExt`
do
    basefilename=$(basename "$filename" .gz);	#gives the filename without extension
	#check if conversion can be started and if not already converted
	if [ $count -gt 0 ] && [ ! -f "$CONVERTED_DATA_DIR$basefilename" ] && [ ! -f "$CONVERTED_DATA_DIR$basefilename"".gz" ] ; then
		
		$DATA_CONVERTER_EXEC $filename $YYYYMMDD $CONVERTED_DATA_DIR$basefilename >> $LOG_FILE 2>>$LOG_FILE &	#start this thread in background
		((count=count-1))
	else
		echo "converting aborted: already converted $basefilename"  >> $LOG_FILE 2>>$LOG_FILE;
	fi
	if [ $count -eq 0 ] ; then	#started all 35 threads, wait for some task to finish
		while [ 1 ] ; do
			sleep 2
			remaining=`ps -ef | grep "nse_generic_to_dotex_offline_converter" | grep -v "grep" | wc -l`
			((count=total-remaining))
			if [ $count -gt 0 ] ; then 
				break
			fi
		done	
	fi
done

echo "EMAIL  Converted all GENERIC at `date`" >> $LOG_FILE 2>>$LOG_FILE

#now we need to zip all the converted files
#at most 35 threads would be running to convert the data now
#lets wait for all conversion to complete

while [ 1 ] ; do
	sleep 2
	remaining=`ps -ef | grep "nse_generic_to_dotex_offline_converter" | grep -v "grep" | wc -l`
	if [ $remaining -eq 0 ] ; then 	#no conversion running
		break
	fi
done

echo "EMAIL start gzip all converted files at `date`" >> $LOG_FILE 2>>$LOG_FILE
for filename in `ls $CONVERTED_DATA_DIR*$YYYYMMDD`
do
	gzip -f $filename >> $LOG_FILE 2>>$LOG_FILE #zip all files
done

echo "EMAIL gzipped all converted files at `date`" >> $LOG_FILE 2>>$LOG_FILE

##check if all files are converted successfully
total_files_in_generic=`ls -lrt /spare/local/MDSlogs/GENERIC/NSE*$YYYYMMDD$fileExt | wc -l`;
total_files_converted=`ls -lrt $CONVERTED_DATA_DIR*$YYYYMMDD.gz | wc -l`;

if [ $total_files_in_generic -eq $total_files_converted ]; then
	echo "EMAIL $total_files_converted files successfully converted"	>> $LOG_FILE 2>>$LOG_FILE
else
	echo "EMAIL Generic to dotex conversion failed" >> $LOG_FILE 2>>$LOG_FILE
	echo "EMAIL Total Files: $total_files_in_generic Files Converted: $total_files_converted" >> $LOG_FILE 2>>$LOG_FILE
fi

#marking data conversion as complete
update_dcstatus_remote "setStatus" "6" "109" "$YYYYMMDD" "10.23.115.61"

#email the stats
egrep "FATAL|EMAIL" $LOG_FILE | awk -F"EMAIL" '{print $2}' | /bin/mail -s "$YYYYMMDD : NSE_GENERIC Data Conversion" "abhishek.anand@tworoads.co.in" "rahul.kumar@tworoads.co.in"
