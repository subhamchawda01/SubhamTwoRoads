crontab -l | grep -v "#" | grep -i STOP | awk '{$1=$2=$3=$4=$5=""}1' | while read entry; 
do
  $entry;
done
