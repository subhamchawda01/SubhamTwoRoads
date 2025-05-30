#!/bin/bash
worker2UserIp="dvctrader@44.202.186.243"
indMachineSimIDSimFileRecord="/tmp/indMachineSimIdSimFileRecord.txt"
workerSimResultsDir="/spare/local/logs/tradelogs"
sim_out="/home/dvctrader/important/SIM_OUT_RESULT/"
> $indMachineSimIDSimFileRecord

mail_and_exit(){
   echo "$1 " | mailx -s "SIM Gen Failure on Worker" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in;
   exit
}

declare -A server_to_ip_map
server_to_ip_map=( 
["IND16"]="10.23.227.81"     
["IND17"]="10.23.227.82" 
["IND18"]="10.23.227.83"
["IND23"]="10.23.227.72"
["IND15"]="10.23.227.65"
["IND19"]="10.23.227.69"
)

check_dest_alive(){
  while true;do
    ping -c1 -w 10 $1 >/dev/null 2>&1
    if [ $? -ne 0 ] ; then
      sleep 1m; echo "Retrying the connection"; continue;
    fi
    break;
  done
}

syncWorkerSimResultsToLocal(){
   while IFS= read -r line;
   do
      server=`echo "$line" | awk '{print $1}'`;
      simId=`echo "$line" | awk '{print $3}'`;
      date=`echo "$line" | awk '{print $2}'`;
      simResultFile="$workerSimResultsDir/log.$date.$simId"
      mkdir -p "/home/dvctrader/important/CASH_SIM_Result/$server/"
      echo "************syncing sim results- $server $date $simId from worker to local***********"
      echo "rsync -avz $worker2UserIp:$simResultFile /home/dvctrader/important/CASH_SIM_Result/$server/"
      rsync -avz "$worker2UserIp:$simResultFile" "/home/dvctrader/important/CASH_SIM_Result/$server/"
   done < $indMachineSimIDSimFileRecord
}


if [ $# -ne 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 Date" ;
  exit;
fi

today=$1
if [ $today == "TODAY" ]; then
        today=`date +"%Y%m%d"`
fi


for server in "${!server_to_ip_map[@]}";do
   echo;echo "--------------------- SERVER: $server -------------------------------"
   sourceAddr="dvctrader@${server_to_ip_map[$server]}"
   tmpvar=`ssh $sourceAddr "crontab -l | egrep  'run.sh|vwap_615' | grep -v '#'" | awk '{print $6}'` 

#   echo "Server: $server Run Scripts: $tmpvar"
   tmpruncommand=`ssh $sourceAddr "echo '$tmpvar' | xargs cat | grep -v '#' | egrep 'start'"`
   echo "Trade engine Run: $tmpruncommand"
   runUniqueId=`echo "$tmpruncommand" | awk '{print $6}'`
# echo "unique runids- $runUniqueId"
   tmpFolders=`echo "$tmpruncommand" | awk '{print $2}' | awk 'BEGIN{FS=OFS="/"}{NF--;print}'| sort | uniq`
#   echo "FOLDER: $tmpFolders" 
   echo "*****************Syncing $server to Local dir ****************"
   for folderPath in $tmpFolders;do
       check_dest_alive "${server_to_ip_map[$server]}"
       dir_name=`echo $folderPath  | rev | cut -d'/' -f1 | rev`
       simrundir="/home/dvctrader/important/CASH_SIM_RUN/${server}/${dir_name}/"
       mkdir -p $simrundir
       echo "Server Alive ${server_to_ip_map[$server]} Folder: $folderPath DIR_USED: $simrundir"
       j=0
       while true; do
          echo "Running Sync rsync -avz $sourceAddr:${folderPath}/ $simrundir "
          rsync -avz "$sourceAddr:${folderPath}/" "$simrundir"
          if [ $? -eq 0 ];
          then
             break;
          fi
          if [ $j -eq 10 ];
          then
             echo "Problem syncing $server data $folderPath to Local dir"
             mail_and_exit "Problem syncing $server data $folderPath to Local dir"
          fi
          j=$((j+1))
       done
   done
   
   echo "******************syncing  local dir $server to worker1************ "
   for folderPath in $tmpFolders;do
      dir_name=`echo $folderPath  | rev | cut -d'/' -f1 | rev`
      simrundir="/home/dvctrader/important/CASH_SIM_RUN/$server/${dir_name}/"
      echo "Local Dir: $simrundir Worker Dir: $simrundir"
      ssh $worker2UserIp "mkdir -p $simrundir"
      j=0;
      while true; do
          echo "Running Sync"
          echo "rsync -avz $simrundir $worker2UserIp:${simrundir}"
          rsync -avz "$simrundir" "$worker2UserIp:${simrundir}"
          if [ $? -eq 0 ];
          then
             break;
          fi
          if [ $j -eq 10 ];
          then
             echo "Problem syncing local data $simrundir to worker2 dir CASH_SIM_RUN"
             mail_and_exit "Problem syncing local data $simrundir to worker2 dir CASH_SIM_RUN"
          fi
          j=$((j+1))
      done
   done
   echo "*********************running simulation for $server data on $worker2UserIp ***********************"
   for runUniqId in $runUniqueId;do
      simCsvFilePath=`echo "$tmpruncommand" |grep -w "$runUniqId" | awk '{print $2}'| awk 'BEGIN{FS=OFS="/"}{print $(NF-1)"/"$NF}'`
      echo "$server $today $runUniqId" >>$indMachineSimIDSimFileRecord
      startTime=`echo "$tmpruncommand" |grep -w "$runUniqId" | awk '{print $4}'`
      endTime=`echo "$tmpruncommand" |grep -w "$runUniqId" | awk '{print $5}'`
      simrundir="/home/dvctrader/important/CASH_SIM_RUN/$server/$simCsvFilePath"
      echo "SimulationOnCsv: $simrundir StartTime: $startTime EndTime: $endTime SimId: $runUniqId"
      sim_out_file="$sim_out/sms.$today.$runUniqId"

      echo "ssh $worker2UserIp /home/dvctrader/stable_exec/trade_engine $simrundir $today $startTime $endTime $runUniqId >$sim_out_file 2>&1 &"
     ssh $worker2UserIp "/home/dvctrader/stable_exec/trade_engine $simrundir $today $startTime $endTime $runUniqId" >$sim_out_file 2>&1 &
   done
   sleep 1m
   tradeEngineInstance=`ssh $worker2UserIp "ps aux | grep  /home/dvctrader/stable_exec/trade_engine | grep -v grep | wc -l"`
   echo "$server TradeEngineInstance= $tradeEngineInstance"
   while [ $tradeEngineInstance -gt 0 ];do
      sleep 15s
      tradeEngineInstance=`ssh $worker2UserIp "ps aux | grep /home/dvctrader/stable_exec/trade_engine | grep -v grep | wc -l"`
      echo "$server TradeEngineInstance= $tradeEngineInstance"
   done

   echo "All current trade_engine jobs Completed"

done

syncWorkerSimResultsToLocal

echo "GENERATE PNL REPORT--- "
/home/pengine/prod/live_scripts/generate_pnl_for_sim_run.sh DAILY $today

echo "" | mailx -s "SIM Pnl generated For $today on Worker" -r "${HOSTNAME}-${USER}<subham.chawda@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in, subham.chawda@tworoads-trading.co.in, infra_alerts@tworoads-trading.co.in;


