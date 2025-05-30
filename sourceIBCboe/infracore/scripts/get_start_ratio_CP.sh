#!/bin/bash

servers="IND11 IND12 IND13 IND14 IND15 IND16 IND17 IND18 IND19 IND20"
date=`date +%Y%m%d`
echo "date : $date"


  declare -A IND_Server_ip
  IND_Server_ip=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND17"]="10.23.227.82" \
                ["IND16"]="10.23.227.81" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
                ["IND20"]="10.23.227.84")


/home/pengine/prod/live_execs/combined_user_msg  --dump_mds_files 0
sleep 10
#ind17_strat_id=`grep -i START_RATIO /home/dvctrader/ATHENA/run.sh | awk '{print $6}'`

true>/tmp/SAOS_CP_mail

for server in "${!IND_Server_ip[@]}";
do
  echo "for ---- $server";
true>/tmp/crontab_files
true>/tmp/run_content
true>/tmp/SYM_SACI
true>/tmp/SAOS_CP
#if [ "$server" == "IND15" ] || [ "$server" == "IND19" ] || [ "$server" == "IND20" ] || 
    if [ "$server" == "IND16" ] || [ "$server" == "IND17" ] || [ "$server" == "IND18" ]; 
# if [ "$server" == "IND17" ];
    then
      echo "entered"
      ssh dvctrader@${IND_Server_ip[$server]} "crontab -l | grep -v '^#' | grep run.sh">/tmp/crontab_files
      files=`cat /tmp/crontab_files | awk '{print $6}'`
      echo -e "for $server\n"

      for file in $files;
      do
        echo -e " $file->\n"

        ssh dvctrader@${IND_Server_ip[$server]} "cat $file | grep -v '^#' | grep 'START_RATIO'"> /tmp/run_content

        echo "runcontent::"
        cat /tmp/run_content
	strats=`awk '{print $6}' /tmp/run_content`
        for strat in $strats;
	do
	  echo "strat: $strat"
	   ssh dvcinfra@${IND_Server_ip[$server]} "grep RMC_SACI /spare/local/logs/tradelogs/log.$date'.'$strat | awk '{print \$4,\$6}'"> /tmp/SYM_SACI
           echo "$server" >> /tmp/SAOS_CP
           while read line
	   do
             
	     SYM=`echo $line | cut -d' ' -f 1`
	     SACI=`echo $line | cut -d' ' -f 2`
	     echo "SYM, SACI:: $SYM, $SACI"
	     SAOS_CP=`LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1 /home/pengine/prod/live_execs/mds_log_reader GENERIC /spare/local/MDSlogs/GENERIC/ORS_$SYM'_'$date | grep "SACI: $SACI "  | tail -1 | awk '{print $2,$13,$14,$19,$20,$25,$26}' | grep -v "CP: 0"`
	   #  if [[ $SAOS_CP == *"CP: 0"* ]]; then
	   #     echo "It's there $SAOS_CP"
	   #  else
#		 echo "nothing there"
#	     fi
             echo " $SAOS_CP" >> /tmp/SAOS_CP
	   done < /tmp/SYM_SACI
 	   cat /tmp/SAOS_CP | awk 'NF >0' >> /tmp/SAOS_CP_mail
	done
      done
    fi
done

if [ `cat /tmp/SAOS_CP_mail | wc -l` -gt 3 ]; then
cat /tmp/SAOS_CP_mail | mail -s "START RATIO CP ALERT: $date" hardik.dhakate@tworoads-trading.co.in uttkarsh.sarraf@tworoads.co.in nishit.bhandari@tworoads-trading.co.in ravi.parikh@tworoads.co.in raghunandan.sharma@tworoads-trading.co.in 
fi
