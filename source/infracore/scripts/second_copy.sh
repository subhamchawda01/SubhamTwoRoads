#!/bin/bash
#runs on 5.42
date=`date +"%Y%m%d"`
if [ $# -eq 1 ];then
  date=$1
fi

is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $date T`
if [ "$is_holiday" == "1" ];then
  exit;
fi

second_copy_report=/tmp/second_copy_report.txt
copy_progress="/tmp/second_copy_progress.txt"
rm -rf $copy_progress
rm -rf $second_copy_report
mountpoint=`df -a | grep "/run/media/dvcinfra/BACKUP"`
usage=`echo $mountpoint | awk '{print $5}' | awk 'BEGIN{FS="%"}{print $1}'`
[ "$mountpoint" == "" ] && echo "DISK NOT MOUNTTED => /dev/sdb2 " >> $second_copy_report
[ $usage -ge 90 ] && echo "DISK USAGE IS MORE THAN 90 % => /dev/sdb2" >>  $second_copy_report

if [ -f $second_copy_report ];then
	cat $second_copy_report | mailx -s "SECOND BACKUP" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
	exit
fi

echo "Syncing NSEMidTerm Data"
i=0
while true ; do
	i=$((i+1))
        [[ $i > 4 ]] && { echo "Syncing Error in MidTerm Data 5.66" >> $copy_progress; break; }
	rsync -avz --progress 10.23.5.66:/NAS1/data/NSEMidTerm/  /NAS1/BACKUP/NSEMidTerm/ >/dev/null 2>/dev/null
	[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
	break;
done

echo "Syncing Bar Data"
i=0
while true ; do 
	i=$((i+1))
        [[ $i > 4 ]] && { echo "Syncing Error in Bar Data 5.66" >> $copy_progress; break; }
	rsync -avz --progress 10.23.5.66:/NAS1/data/NSEBarData/ /NAS1/BACKUP/NSEBarData/ >/dev/null 2>/dev/null
	[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
        break;
done

echo "Syncing LatencySystems"
i=0
while true; do
	i=$((i+1))
        [[ $i > 4 ]] && { echo "Syncing Error in LatencySystems 5.67" >> $copy_progress; break; }
	rsync -avz --progress 10.23.5.67:/NAS1/data/LatencySystem/www/ /NAS1/BACKUP/LatencySystem >/dev/null 2>/dev/null
	[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
        break;
done

echo "Syncing Pnl Reports"
i=0
while true; do
	i=$((i+1))
        [[ $i > 4 ]] && { echo "Syncing Error in Pnl Pnl 5.67" >> $copy_progress; break; }
	rsync -avz --progress 10.23.5.67:/NAS1/data/PNLReportsIND/ /NAS1/BACKUP/PNLReportsIND >/dev/null 2>/dev/null
	[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
        break;
done


echo "Syning Backup Ors Data"
i=0
while true; do
	i=$((i+1))
        [[ $i > 4 ]] && { echo "Syncing Error in ORS Data" >> $copy_progress; break; }
	rsync -avz --progress 10.23.5.67:/NAS1/data/ORSData/NSE  /NAS1/BACKUP/ORSDATA/NSE/ >/dev/null 2>/dev/null
	[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
        break;
done

echo "Syncing ORSBCAST_MULTISHM Q19"
ssh 10.23.227.63 "find /spare/local/ORSBCAST_MULTISHM/NSE/ -type f | grep -v ".gz" | xargs gzip"
i=0
while true; do 
	i=$((i+1))
	[[ $i > 4 ]] && { echo "Syncing Error in ORSBCAST_MULTISHM Q19 IND13" >> $copy_progress; break; }
	rsync -avz --progress 10.23.227.63:/spare/local/ORSBCAST_MULTISHM/NSE/ /NAS1/BACKUP/ORSBCAST_MULTISHM_Q19 >/dev/null 2>/dev/null
	[ $? -ne 0 ]  && { echo "Retrying..."; sleep 10m; continue;  }
        break;
done


#ssh 10.23.227.63 "find /spare/local/ORSBCAST_MULTISHM_S7/ -type f | grep -v ".gz" | xargs gzip"
#echo "Syncing MUltishmS7"
#while true; do
#	rsync -avz --progress 10.23.5.67:/spare/local/ORSBCAST_MULTISHM_S7/ /NAS1/BACKUP/ORSDATA/ORSBCAST_MULTISHM_S7/ >/dev/null 2>/dev/null
#	[ $? -ne 0 ]  && { echo "Retrying..."; sleep 10m; continue;  }
#       break;
#done


#tradeinfo
mount_point=`df -a | grep "/run/media/dvcinfra/NonMDBackup"`
usage=`echo $mount_point | awk '{print $5}' | awk 'BEGIN{FS="%"}{print $1}'`
[ "$mount_point" == "" ] && echo "Disk is not mounted NONMdBackup " >> $second_copy_report
[ $usage -ge 90 ] && echo "Disk usage is more than 90 on NONMdBackup" >> $second_copy_report

if [ -f $second_copy_report ];then
        cat $second_copy_report | mailx -s "SECOND BACKUP" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
        exit
fi

echo "Syncing trade from 5.13"
i=0
while true; do
	i=$((i+1))
        [[ $i > 4 ]] && { echo "Syncing Error in TradeInfo 5.13" >> $copy_progress; break; }
	rsync -avz --progress 10.23.5.13:/spare/local/tradeinfo/ /run/media/dvcinfra/NonMDBackup/tradeinfo/ >/dev/null 2>/dev/null
	[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
        break;
done

#compressed tradelogs and orslogs
echo "Syncing Server Logs"
declare -a servers
servers=("IND11" "IND12" "IND13" "IND14" "IND15" "IND16" "IND17" "IND18" "IND19" "IND20");	
for server in "${servers[@]}";do
	echo "Syncing Orslogs for $server"
	i=0
	while true; do
		i=$((i+1))
	        [[ $i > 4 ]] && { echo "Syncing Error in ORSlogs 5.13" >> $copy_progress; break; }
		rsync -avz --progress 10.23.5.13:/run/media/root/Elements/SERVERDATA/$server/ORSlogs /run/media/dvcinfra/NonMDBackup/SERVERDATA/$server >/dev/null 2>/dev/null
		[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  }
        	break;
	done
	echo "Syncing tradelog for $server"
	i=0
	while true; do
		i=$((i+1))
                [[ $i > 4 ]] && { echo "Syncing Error in TradeLogs 5.13" >> $copy_progress; break; }
		rsync -avz --progress 10.23.5.13:/run/media/root/Elements/SERVERDATA/$server/tradelogs /run/media/dvcinfra/NonMDBackup/SERVERDATA/$server >/dev/null 2>/dev/null
		[ $? -ne 0 ] && { echo "Retrying..."; sleep 10m; continue;  } 
		break;
	done
done

if [ -f $copy_progress ];then
        cat $copy_progress | mailx -s "SECOND BACKUP" -r "${HOSTNAME}-${USER}<sanjeev.kumar@tworoads-trading.co.in>" sanjeev.kumar@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
fi

