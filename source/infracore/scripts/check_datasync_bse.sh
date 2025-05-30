today=$1
yyyy=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}
mailfile=/tmp/mailfile;
>$mailfile;

remote_data_dir=/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/
local_data_dir=/NAS1/data/BSELoggedData/BSE/${yyyy}/${mm}/${dd}/

remote_file_count=`ssh dvcinfra@192.168.132.12  "ls -lrt ${remote_data_dir}  | egrep -v \"_PE_|_CE_\" | wc -l"`
local_file_count=`ls -lrt ${local_data_dir} | egrep -v "_PE_|_CE_" | wc -l`;

#if all the files are not synced, then start sync 
if [ $remote_file_count -ne $local_file_count ];
then
  echo "REMOTE FILE COUNT : ${remote_file_count}" > $mailfile
  echo "LOCAL FILE COUNT : ${local_file_count}" >> $mailfile
  #check status of the line
  cat $mailfile | mailx -s "BSE DATA COPY PROBLEM:Trying Again" -r -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in naman.jain@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  rsync -avz --progress  --exclude="*_CE_*" --exclude="*_PE_*"  192.168.132.12:${remote_data_dir} ${local_data_dir}
fi


remote_file_count=`ssh dvcinfra@192.168.132.12  "ls -lrt ${remote_data_dir}  | egrep -v \"_PE_|_CE_\" | wc -l"`
local_file_count=`ls -lrt ${local_data_dir} | egrep -v "_PE_|_CE_" | wc -l`;

if [ $remote_file_count -ne $local_file_count ];
then
  echo "" | mailx -s "BSE DATACOPY PROBLEM : FAILED TO SYNC ALL DATA" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in naman.jain@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit -1
else
  echo "" | mailx -s "BSE DATACOPY, FILES SYNCED SUCCESSFULLY : ${remote_file_count} : ${local_file_count}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"  sanjeev.kumar@tworoads-trading.co.in  ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in naman.jain@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
fi

#now data is synced, sync that to worker assuming again connection break doesn't happen
rsync -avz --progress --exclude="*_CE_*" --exclude="*_PE_*" ${local_data_dir}  dvctrader@44.202.186.243:${local_data_dir}

files_count=`ssh dvctrader@44.202.186.243 "ls -lrt /NAS1/data/BSELoggedData/BSE/$yyyy/$mm/$dd/ | egrep -v \"_PE_|_CE_\" | wc -l"` ;
echo "worker_count: $files_count, $remote_file_count"
if [ $files_count -lt $remote_file_count ];
then
  echo "" | mailx -s "BSE DATACOPY PROBLEM : FAILED TO SYNC DATA TO WORKER" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>"   sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in naman.jain@tworoads-trading.co.in tarun.joshi@tworoads-trading.co.in infra_alerts@tworoads-trading.co.in
  exit -1
fi
