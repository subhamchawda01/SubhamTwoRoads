#! /bin/bash
weekly_config="/home/pengine/prod/live_configs/weekly_update_config";
update_script="/home/pengine/prod/live_scripts/update_production_setup.sh";
temp_update_file="/home/pengine/prod/temp_weekly_update_input";
>$temp_update_file

while read -r line
do
  server_list=`echo $line | awk -F"~" '{print $1}'`;
  [ ${server_list:0:1} == "#" ] && continue; #ignore the first line
  exec_list=`echo $line | awk -F"~" '{print $2}'`;
  echo "7" >> $temp_update_file
  echo "$exec_list" >> $temp_update_file
  echo "master" >> $temp_update_file
  echo "bot" >> $temp_update_file
  echo "WeeklyUpdate" >> $temp_update_file
  echo "$server_list" >> $temp_update_file
  echo "N" >> $temp_update_file

  cat $temp_update_file | $update_script
  >$temp_update_file
done < /home/pengine/prod/live_configs/weekly_update_config
