#!/bin/bash
#Script to generate Min Bar data for a give shortcode/file containing shortcodes
if [[ $# -lt 3 ]]; then
  echo "USAGE: $0 <shortcode/file> <start-date> <end-date> [regenerate=0/1, default:0]"
  exit 1
fi

shc=$1
start_date=$2
end_date=$3
exec_path="$HOME/LiveExec/bin"

regenerate=0
if [[ $# -gt 3 ]]; then
  regenerate=$4
fi

#Prepare an array of shortcodes
shc_array=()
if [ -f $shc ]; then
  for shortcode in `cat $shc`;
  do
    shc_array+=("$shortcode");
  done
else
  shc_array+=("$shc");
fi

#Loop over the given range of days
generated=0
while [ "$start_date" -le "$end_date" ];
do
  echo "Generating for $start_date"
  rm -f /spare/local/MDSlogs/MinuteBar/*$start_date*
  generated=0
  for shortcode in "${shc_array[@]}";
  do
    echo "shortcode: $shortcode $regenerate"
    if [ "$regenerate" == "0" ]; then
      exch_symbol=`$exec_path/get_exchange_symbol $shortcode $start_date`
      filename="/NAS1/data/MinuteBarLoggedData/COMMON/${start_date:0:4}/${start_date:4:2}/${start_date:6:2}/MB_${exch_symbol}_$start_date.gz"
      if [ -f $filename ]; then
        echo "Not generating as file already present: $filename"
        continue
      fi
    fi
    #Everything clear. Generating min-bar data for this shortcode and date
    echo "Running $exec_path/generate_bardata SIM $shortcode $start_date /spare/local/MDSlogs/MinuteBar/"
    $exec_path/generate_bardata SIM $shortcode $start_date /spare/local/MDSlogs/MinuteBar/
    generated=1
  done
  if [ "$generated" == "1" ]; then
    echo "Result generation for $start_date done. Copying data"
    $HOME/LiveExec/scripts/mds_log_backup.sh MinuteBar COMMON $start_date >/dev/null 2>&1
  fi
  rm -f /spare/local/MDSlogs/MinuteBar/*$start_date*
  start_date=`$exec_path/calc_next_day $start_date`
done
