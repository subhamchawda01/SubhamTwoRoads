#!/bin/bash
mkdir -p /spare/local/pnl_delta/

LAST_TRADE_FILE="/spare/local/pnl_delta/last_trades_.txt";
LAST_TRADE_FILE_MFT="/spare/local/pnl_delta/last_trades_mft_.txt";
DELTA_FILE="/spare/local/pnl_delta/delta_trades_.txt";
EACH_FILE="/spare/local/pnl_delta/each_file_.txt";
FILE="/spare/local/pnl_delta/fetched_file_.txt";
LOG_FILE="/spare/local/pnl_delta/delta_service_logs";
added_lines="/spare/local/pnl_delta/added_lines";
deleted_lines="/spare/local/pnl_delta/deleted_lines";
added_lines_tmp="/spare/local/pnl_delta/added_lines_tmp";

# It was observed in case of server restart, get-delta_trades.sh resends duplicate trades(common issue on CHI13).
# This update is a fix for the above mentioned issue, by providing a notification to NY11 about service restart.
# The setup sends a dummy file with timestamp which act as a notification.

curr_time=$(date +%Y%m%d%H%M%S);
this_host_ip=$(hostname | awk -F"." '{print $1}');
echo "#Get Delta-trade service restarted" > /spare/local/pnl_delta/${this_host_ip}_restart_$curr_time;
trade_dir="hft"
rsync -avz /spare/local/pnl_delta/${this_host_ip}_restart_$curr_time dvcinfra@10.23.74.51:/spare/local/logs/pnl_data/"$trade_dir"/delta_files 2>> $LOG_FILE;
sleep 120

if [ ! -f $LAST_TRADE_FILE ] ; then
      touch $LAST_TRADE_FILE
fi

if [ ! -f $LAST_TRADE_FILE_MFT ] ; then
      touch $LAST_TRADE_FILE_MFT
fi

if [ ! -f $DELTA_FILE ] ; then
     touch $DELTA_FILE
fi  

if [ ! -f $FILE ] ; then
     touch $FILE
fi

if [ ! -f $EACH_FILE ] ; then
     touch $EACH_FILE
fi

>$LAST_TRADE_FILE
>$LAST_TRADE_FILE_MFT
>$DELTA_FILE
>$FILE

#lets define what all servers are trading in MFT
#add here if we start mft on any new machine. For now, only on ind (substring of hostname).
mft_servers="ind";

#select the time when you switch to next day and start fetching next's day trade files
switch_next_day_trade_config=/home/pengine/prod/live_configs/switch_to_next_day_trades.txt

flag=0;


#Idea is here to get all deleted lines and remove its corresponding added line. Think of how 'diff' works.
# All the added lines that remain are the delta lines.
# This handling is done if there are multiple trade files fetched from same server. For eg: retail trades in case
# of MICEX

calc_delta() {
#symbol name may contain spaces, replace them with # for now
  diff $1 $2 | grep "<" | sed 's/< //g' | sed 's/ /#/g' > $deleted_lines 2>> $LOG_FILE
  diff $1 $2 | grep ">" | sed 's/> //g' | sed 's/ /#/g' > $added_lines 2>> $LOG_FILE

  filename="$deleted_lines"
  while read -r line
  do
     sed "0,/$line/{/$line/d}" $added_lines > $added_lines_tmp 2>> $LOG_FILE # deletes the first occurence of $line
     mv $added_lines_tmp $added_lines 2>> $LOG_FILE
  done < "$filename"
  sed 's/#/ /g' $added_lines > $added_lines_tmp 2>> $LOG_FILE
  mv $added_lines_tmp $added_lines 2>> $LOG_FILE
  cp $added_lines $DELTA_FILE 2>> $LOG_FILE
}
#fetch_trades YYYYMMDD hft/mtt {arguments}
fetch_trades() {
	YYYYMMDD=$1
	shift
	trade_type=$1	#should either be hft or mtt
	shift
	trade_files=$(find /spare/local/ORSlogs/ -name "trades.$YYYYMMDD" | grep -v DC | \
				grep -v FFDVC | grep -v DBRP | grep -v EJG9 | grep trades | $@) 2>> $LOG_FILE
	trade_files_array=($trade_files) 2>> $LOG_FILE
	# echo $trade_files
	# echo $trade_files_array
	>$FILE  
	for file in "${trade_files_array[@]}"
	{
		>$EACH_FILE
		rsync -avz $file $EACH_FILE 2>>$LOG_FILE
		cat $EACH_FILE >> $FILE 2>>$LOG_FILE
	}
	# $FILE has trade contents at current instance of time. It would be some info appended to $LAST_TRADE_FILE
	#grep -Fxvf $LAST_TRADE_FILE $FILE > $DELTA_FILE 2>/dev/null;	#version 1
	#diff $LAST_TRADE_FILE $FILE | grep ">" | sed 's/> //g' > $DELTA_FILE 2>/dev/null; #version 2
	calc_delta $LAST_TRADE_FILE $FILE  2>> $LOG_FILE

	if [ -s "$DELTA_FILE" ] ; then
		curr_time=$(date +%Y%m%d%H%M%S)
		this_delta_file="/spare/local/pnl_delta/delta_$this_host_ip""_$curr_time";
		cp $DELTA_FILE $this_delta_file >> $LOG_FILE 2>> $LOG_FILE

		rsync -avz $this_delta_file dvcinfra@10.23.74.51:/spare/local/logs/pnl_data/"$trade_type"/delta_files/ 2>> $LOG_FILE
		rsync_error=$?;
		if [ "$rsync_error" -eq "0" ] ; then
		   cat $DELTA_FILE >> $LAST_TRADE_FILE 2>> $LOG_FILE
		else
			echo "RSYNC error code:  $rsync_error "
		fi
		rm -f $this_delta_file
	fi
}

fetch_internal_trades() {
	YYYYMMDD=$1
	shift
	trade_type=$1	#should either be hft or mtt
	shift
	internal_trades_file="/spare/local/logs/alllogs/internal_trades.$YYYYMMDD";
	trade_files_array=($internal_trades_file);
	# echo $trade_files
	# echo $trade_files_array
	>$FILE
	for file in "${trade_files_array[@]}"
	{
		>$EACH_FILE
		rsync -avz $file $EACH_FILE 2>>$LOG_FILE
		cat $EACH_FILE >> $FILE 2>>$LOG_FILE
	}
	# $FILE has trade contents at current instance of time. It would be some info appended to $LAST_TRADE_FILE
	#grep -Fxvf $LAST_TRADE_FILE $FILE > $DELTA_FILE 2>/dev/null;	#version 1
	#diff $LAST_TRADE_FILE $FILE | grep ">" | sed 's/> //g' > $DELTA_FILE 2>/dev/null; #version 2
	calc_delta $LAST_TRADE_FILE $FILE  2>> $LOG_FILE

	if [ -s "$DELTA_FILE" ] ; then
		curr_time=$(date +%Y%m%d%H%M%S)
		this_delta_file="/spare/local/pnl_delta/delta_$this_host_ip""_${curr_time}_int_execution";
		cp $DELTA_FILE $this_delta_file >> $LOG_FILE 2>> $LOG_FILE

		rsync -avz $this_delta_file dvcinfra@10.23.74.51:/spare/local/logs/pnl_data/"$trade_type"/delta_files/ 2>> $LOG_FILE
		rsync_error=$?;
		if [ "$rsync_error" -eq "0" ] ; then
		   cat $DELTA_FILE >> $LAST_TRADE_FILE 2>> $LOG_FILE
		else
			echo "RSYNC error code:  $rsync_error "
		fi
		rm -f $this_delta_file
	fi
}

while [ true ]
do

	time_to_switch_=`cat $switch_next_day_trade_config`;
	YYYYMMDD=$(date "+%Y%m%d");
	time_now_=$(date "+%H%M%S");

	if [ "$time_now_" -gt "$time_to_switch_" ] ; then
	   YYYYMMDD=$(date --date='tomorrow' "+%Y%m%d"); 
	   if [ "$flag" -eq "0" ] ; then
		  >$LAST_TRADE_FILE
                  >$LAST_TRADE_FILE_MFT
		  flag=1;
	   fi  
	fi

	if [ "$time_now_" -lt "011111" ] ; then
		  flag=0;
	fi
	fetched_flag=0;
	for server in `echo $mft_servers`
	do
		if [[ "$this_host_ip" =~ "$server" ]] ; then
			fetch_trades $YYYYMMDD hft grep -v MEDFO 2>> $LOG_FILE
			fetch_trades $YYYYMMDD mtt grep MEDFO 2>> $LOG_FILE
			fetch_internal_trades $YYYYMMDD hft grep trades 2>> $LOG_FILE
			fetched_flag=1;
			break;
		fi
	done
	if [ "$fetched_flag" -eq 0 ] ; then
		fetch_trades $YYYYMMDD hft grep trades 2>> $LOG_FILE
		fetch_internal_trades $YYYYMMDD hft grep trades 2>> $LOG_FILE
	fi
sleep 60
done
