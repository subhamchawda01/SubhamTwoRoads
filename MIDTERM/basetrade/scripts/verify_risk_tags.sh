#!/bin/bash
#Script to verify if the assigned risk tags are correct for the pick-strats config files
if [ $# -ge 1 ]; then
  echo "USAGE: $0"
  exit 0
fi

#truncate mail file
mail_file="/spare/local/logs/risk_verification_log"
> $mail_file

for cfg_file in `find /home/dvctrader/modelling/pick_strats_config/ -name "*.txt"`; do
  st=`grep -A 1 EXEC_START $cfg_file | tail -1`
  st=`~/basetrade_install/bin/get_utc_hhmm_str $st`

  tags=`grep -A 1 RISK_TAGS $cfg_file | tail -1`
  expected="dummy"

  #heuristically compute expected tags for this config
  if [[ "$cfg_file" == *NSE_* ]]; then
    #NSE always in EU
    expected="GLOBAL:EU:ASEU:EUUS"
  elif [ "$st" -ge 2130 ] || [ "$st" -lt 500 ]; then
    expected="GLOBAL:AS:ASEU"
  elif [ "$st" -lt 1059 ]; then
    expected="GLOBAL:EU:ASEU:EUUS"
  else
    expected="GLOBAL:US:EUUS"
  fi

  #Check if expected and assigned tags match
  if [ ! "$expected" == "dummy" ] && [ ! "$expected" == "$tags" ]; then
    echo "Issue with $cfg_file; Expected: $expected, Configured: $tags, StartTime: $st" >> $mail_file
  fi
done

if [ -s $mail_file ]; then
    /bin/mail -s "Risk Tags Discrepancy" -r nseall@tworoads.co.in chandan.kumar@tworoads.co.in puru.sarthy@circulumvite.com < $mail_file
fi
