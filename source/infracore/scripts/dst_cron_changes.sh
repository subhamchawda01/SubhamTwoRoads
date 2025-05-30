#!/bin/bash
#This script generates the updated crontab based on the provided hourly increment (+1/-1)
#This assumes that for processes outside the time range [2300, 0100], we dont need to make any changes, as these are our
#processes whihc are not dependent on exchange timings. For others it simply adds that increment

if [ $# -ne 1 ]; then
  echo "$0 -1/1  (-1:prepone by 1 hour, 1:postpone by 1 hour)"
  exit 0
fi

#Validate the increment
diff=$1
if [ "$diff" != "-1" ] && [ "$diff" != "+1" ]; then
  echo "Increment $diff is not supported. Use -1 or +1"
  exit 0
fi

#Print the updated crontab
crontab -l | awk -v increment=$diff '{if (($2 ~ /^[0-9]+$/) && ($2 != "*") && ($2 > 2) && ($2 < 23)) $2 = $2 + increment; print $0;}'
