FILE=/home/pengine/prod/live_configs/data_copy.cfg
PREFIX=/NAS1/data
STARTDATE=20160101
S3_CMD_EXEC="/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd"
LOOKBACK=10
SCRIPTS_PATH=$HOME/basetrade/scripts
while IFS='' read -r line || [[ -n "$line" ]]; do
  line=`echo $line | grep -v '#'`;
  if [ "$line" != "" ]; then

  exch=`echo $line | cut -d' ' -f3`
  loc=`echo $line | cut -d' ' -f1`
  if [ "$exch" == "ORS" ]; then
    path=$PREFIX/"$exch"Data/$loc
  else
    path=$PREFIX/"$exch"LoggedData/$loc
  fi

  for i in `seq 0 $LOOKBACK`; do #Sync for last 200 days
    date_path=`date --date="-$i day" +%Y/%m/%d`
    full_path=$path/$date_path
		/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd sync $full_path/ s3://s3dvc$full_path/
  done
  fi
done < "$FILE"
