#!/bin/sh

for file in /data3/HKEXRawStocksData/HKEXStocksData/* ; do
	
	DATE=$(echo `basename $file` | cut -d'_' -f2 | cut -d'.' -f1) ;

	echo "Running for $DATE..Copying to  NY" ;

	cp $file /home/vedant/HKEXStocks/ ;
	cd /home/vedant/HKEXStocks/ ; unzip `basename $file` ;
	
	sleep 3 ;
	echo "Unzipped Files...Trying to create local MDSlogs.." ;
	
 	/home/vedant/infracore_install/bindebug/hk_hist_offline_decoder /home/vedant/HKEXStocks/ $DATE CONVERT >~/out.$DATE >~/out_err.$DATE ;

	echo "Local MDSlogs for created..Uploading PF to NAS and S3" ;

	~/LiveExec/scripts/mds_log_backup.sh HKStocksPF HK $DATE ;

	echo "Uploading OF to NAS and S3" ;

	~/LiveExec/scripts/mds_log_backup.sh HKStocks HK $DATE ;
	
	echo "Uploading Completed.." ;

	rm -rf /spare/local/MDSlogs/HKStocksPF/* ;
	rm -rf /spare/local/MDSlogs/HKStocks/* ;

	echo "Removed local MDSlogs.." ;
	
	rm -rf /home/vedant/HKEXStocks/* ;

	echo "Removed Unzipped and Raw Files on NY" ;

done;

	
