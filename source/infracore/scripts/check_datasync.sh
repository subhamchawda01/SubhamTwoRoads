today=$1
yyyy=${today:0:4}
mm=${today:4:2}
dd=${today:6:2}
mailfile=/tmp/mailfile;
>$mailfile;

remote_data_dir=/spare/local/MDSlogs/${yyyy}/${mm}/${dd}/
local_data_dir=/NAS1/data/NSELoggedData/NSE/${yyyy}/${mm}/${dd}/

remote_file_count=`ssh dvcinfra@10.23.227.63  "ls -lrt ${remote_data_dir}  | egrep -v \"_PE_|_CE_\" | wc -l"`
local_file_count=`ls -lrt ${local_data_dir} | egrep -v "_PE_|_CE_" | wc -l`;

#if all the files are not synced, then start sync 
if [ $remote_file_count -ne $local_file_count ];
then
  echo "REMOTE FILE COUNT : ${remote_file_count}" > $mailfile
  echo "LOCAL FILE COUNT : ${local_file_count}" >> $mailfile
  #check status of the line
  cat $mailfile | mailx -s "DATA COPY PROBLEM:Trying Again" -r $HOSTNAME  sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
  rsync -avz --progress  --exclude="*_CE_*" --exclude="*_PE_*"  10.23.227.63:${remote_data_dir} ${local_data_dir}
fi


remote_file_count=`ssh dvcinfra@10.23.227.63  "ls -lrt ${remote_data_dir}  | egrep -v \"_PE_|_CE_\" | wc -l"`
local_file_count=`ls -lrt ${local_data_dir} | egrep -v "_PE_|_CE_" | wc -l`;

if [ $remote_file_count -ne $local_file_count ];
then
  echo "" | mailx -s "DATACOPY PROBLEM : FAILED TO SYNC ALL DATA" -r $HOSTNAME  sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
  exit -1
else
  echo "" | mailx -s "DATACOPY, FILES SYNCED SUCCESSFULLY : ${remote_file_count} : ${local_file_count}" -r $HOSTNAME  sanjeev.kumar@tworoads-trading.co.in  ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
fi

#now data is synced, sync that to worker assuming again connection break doesn't happen
rsync -avz --progress ${local_data_dir}  dvctrader@54.90.155.232:${local_data_dir}

files_count=`ssh 54.90.155.232 "ls -lrt /NAS1/data/NSELoggedData/NSE/$yyyy/$mm/$dd/ | egrep -v \"_PE_|_CE_\" | wc -l"` ;
echo "worker_count: $files_count, $remote_file_count"
if [ $files_count -lt $remote_file_count ];
then
  echo "" | mailx -s "DATACOPY PROBLEM : FAILED TO SYNC DATA TO WORKER" -r $HOSTNAME   sanjeev.kumar@tworoads-trading.co.in ravi.parikh@tworoads.co.in nishit.bhandari@tworoads.co.in uttkarsh.sarraf@tworoads.co.in hardik.dhakate@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in 
  exit -1
fi
