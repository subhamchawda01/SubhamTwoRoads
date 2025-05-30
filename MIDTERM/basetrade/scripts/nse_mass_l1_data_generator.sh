#!/bin/bash

# Input $1 = File containing list of Dates (YYYYMMDD).

for  date in `cat $1 | grep -v "#"` ; do

	echo $date 
	YYYY=${date:0:4}
	MM=${date:4:2}
	DD=${date:6:2}
	
	L1_OUT_DIR="/spare/local/MDSlogs/NSEL1/"
	LOG="/spare/local/MDSlogs/NSEL1/NSELog/"$YYYY"/"$MM"/"$DD"/"
	
	mkdir -p $L1_OUT_DIR
	mkdir -p $LOG
	
	count=20
	total=20
	for shc in `~/basetrade_install/bin/get_contract_specs  ALL $date | grep "SHC" | grep "NSE_" | grep -v "FUT0_1" | cut -d' ' -f2 ` ; 
	do 
		if [ $count -gt 0 ] ; then

			echo $shc" !!"   #Identifier so that we can do `grep -v "!!" form log`  
			/home/dvcinfra/LiveExec/bin/l1_data_generator $date $shc $L1_OUT_DIR > $LOG$shc"_"$date 2>&1 &
			((count=count-1))
		fi
		if [ $count -eq 0 ] ; then
			while [ 1 ] ; do
				sleep 2
				remaining=`ps -ef | grep "l1_data_generator" | grep -v "grep" | wc -l`
				((count=total-remaining))
				if [ $count -gt 0 ] ; then 
					break
				fi
			done
			#echo "Remaining " $remaining
			#echo "Starting " $count
		fi
	done
	/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh NSEL1 NSE $date > $LOG"log_mds_backup_"$date 2>&1
	#Notify in case of failure
	if [ `cat $LOG"log_mds_backup_"$date | grep "ERROR" | wc -l` -gt 0 ] ; then
		echo "Please Investigate. Log Dir: $LOG" | /bin/mail -s "$date : NSE_L1 Sync Failed" -r "nse_mass_l1_data_generator" "pranjal.jain@tworoads.co.in , vedant@tworoads.co.in"
	fi
	#Notify in case of success
	#if [ `cat $LOG"log_mds_backup_"$date | grep "SUCCESS" | wc -l` -gt 0 ] ; then
	#	echo "Congratulations." | /bin/mail -s "$date : NSE_L1 Sync Successfull" -r "nse_mass_l1_data_generator" "pranjal.jain@tworoads.co.in , vedant@tworoads.co.in"
	#fi
	#Delete L1 Data, We are intentionally not doing rm -r as we want to delete only files and not directory inside it
	for f in "$L1_OUT_DIR"* ; do rm $f ; done
done

echo "Finished"
