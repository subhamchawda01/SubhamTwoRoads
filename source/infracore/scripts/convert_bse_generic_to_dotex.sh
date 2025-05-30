#!/bin/bash
USAGE="$0 YYYYMMDD convertdir Outdir";
if [ $# -ne 3 ] ;
then
    echo $USAGE
    exit;
fi

DATA_CONVERTER_EXEC="/home/pengine/prod/live_execs/bse_generic_to_dotex_offline_converter";
AFFIN_EXEC="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;
DATA_DIR=$2"/"
CONVERTED_DATA_DIR=$3"/";

YYYYMMDD=$1
LOG_FILE="/spare/local/logs/alllogs/bse_generic_to_dotex_converter.log.$YYYYMMDD";
> $LOG_FILE

fileExt="";
if [ `ls ${DATA_DIR}/BSE*$YYYYMMDD | wc -l` -le 0 ]; then #generic files compressed
	fileExt=".gz";
fi


#Dump MDS files (from CombinedShmWriter) before initiating datacopy
/home/pengine/prod/live_execs/combined_user_msg --dump_mds_files --only_ors_files 0
sleep 10

#delete converted files older than 2 day
find $CONVERTED_DATA_DIR -type f -mtime +2 -exec rm -f {} \;

echo "EMAIL  Starting Conversion all GENERIC at `date`" >> $LOG_FILE 2>>$LOG_FILE
count=20
total=20
for filename in `ls ${DATA_DIR}/BSE*$YYYYMMDD$fileExt`
do
    basefilename=$(basename "$filename" .gz);	#gives the filename without extension
	#check if conversion can be started and if not already converted
	if [ ! -f "$CONVERTED_DATA_DIR$basefilename" ] && [ ! -f "$CONVERTED_DATA_DIR$basefilename"".gz" ] ; then
		$DATA_CONVERTER_EXEC $filename $YYYYMMDD $CONVERTED_DATA_DIR$basefilename >> $LOG_FILE 2>>$LOG_FILE &	#start this thread in background
                DATA_CONV_PID=$! ;
                $AFFIN_EXEC ASSIGN $DATA_CONV_PID "DATACONVERTION" >>$LOG_FILE 2>&1
		((count=count-1))

	else
		echo "converting aborted: already converted $basefilename"  >> $LOG_FILE 2>>$LOG_FILE;
	fi
	if [ $count -eq 0 ] ; then	#started all 35 threads, wait for some task to finish
		while [ 1 ] ; do
			sleep 0.2
			remaining=`ps -ef | grep "bse_generic_to_dotex_offline_converter" | grep -v "grep" | wc -l`
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
	remaining=`ps -ef | grep "bse_generic_to_dotex_offline_converter" | grep -v "grep" | wc -l`
	if [ $remaining -eq 0 ] ; then 	#no conversion running
		break
	fi
done

echo "EMAIL start gzip all converted files at `date`" >> $LOG_FILE 2>>$LOG_FILE
#for filename in `ls $CONVERTED_DATA_DIR*$YYYYMMDD`
#do
#	gzip -f $filename >> $LOG_FILE 2>>$LOG_FILE #zip all files
#done

count=8
total=8
core_alloc=0
echo "EMAIL start gzip all converted files at `date`" >> $LOG_FILE 2>>$LOG_FILE
for filename in `ls $CONVERTED_DATA_DIR*$YYYYMMDD`
do

        taskset -c $core_alloc gzip -f $filename >> $LOG_FILE 2>>$LOG_FILE & #zip all files
        ((count=count-1))
        ((core_alloc=core_alloc+1))
        if [ $core_alloc -eq 8 ] ; then
               core_alloc=0;
        fi
        if [ $count -eq 0 ] ; then      #started all 35 threads, wait for some task to finish
              while [ 1 ] ; do
                      sleep 0.1
                      remaining=`ps -ef | grep "gzip" | grep "BSE_"| grep "$YYYYMMDD" | grep -v "grep" | wc -l`
                     ((count=total-remaining))
                     if [ $count -gt 0 ] ; then
                             break
                     fi
              done
        fi
done

while [ 1 ] ; do
    sleep 2
    remaining=`ps -ef | grep "gzip" | grep "BSE_" | grep "$YYYYMMDD" | grep -v "grep" | wc -l`
    if [ $remaining -eq 0 ] ; then  #no conversion running
            break
    fi
done

echo "EMAIL gzipped all converted files at `date`" >> $LOG_FILE 2>>$LOG_FILE

##check if all files are converted successfully
total_files_in_generic=`ls -lrt ${DATA_DIR}/BSE*$YYYYMMDD$fileExt | wc -l`;
total_files_converted=`ls -lrt $CONVERTED_DATA_DIR*$YYYYMMDD.gz | wc -l`;

echo "GENERIC FILE COUNT : $total_files_in_generic";
echo "FILES CONVERTED : $total_files_converted"; 

if [ $total_files_in_generic -eq $total_files_converted ]; then
	echo "EMAIL $total_files_converted files successfully converted"	>> $LOG_FILE 2>>$LOG_FILE
else
	echo "EMAIL Generic to dotex conversion failed" >> $LOG_FILE 2>>$LOG_FILE
	echo "" | mailx -s "FAILED DATA CONVERSION on $HOSTNAME" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
	echo "EMAIL Total Files: $total_files_in_generic Files Converted: $total_files_converted" >> $LOG_FILE 2>>$LOG_FILE
fi

#email the stats
egrep "FATAL|EMAIL" $LOG_FILE | awk -F"EMAIL" '{print $2}' | /bin/mail -s "$YYYYMMDD : BSE_GENERIC Data Conversion" "raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in "
