#!/bin/bash
file_mail="/tmp/mail_for_the_Geeks_calucualtion_worker"
>$file_mail
GetPreviousWorkingDay() {
  previous_day=`/home/pengine/prod/live_execs/update_date $previous_day P W`
  is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $previous_day T`
  while [ $is_holiday_ = "1" ];
  do
    previous_day=`/home/pengine/prod/live_execs/update_date $previous_day P W`
    is_holiday_=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $previous_day T`
  done
}

#Main 
if [ $# -lt 1 ] ; then
  echo "Called As : " $* ;
  echo "$0 YYYYMMDD" ;
  exit
fi

YYYYMMDD=$1;
previous_day=$YYYYMMDD
GetPreviousWorkingDay
echo "Previous Working Day: $previous_day"
TYPE_="Y"
[[ $2 == "N" ]] && TYPE_='N'
monthly_="/spare/local/MDSlogs/MediumTermOptions/"
weekly_="/spare/local/MDSlogs/MediumTermWeeklyOptions/"

local_m="/spare/local/MIDTERM_DATA/MediumTermOptions/"
local_w="/spare/local/MIDTERM_DATA/MediumTermWeeklyOptions/"

echo "TYPE $TYPE_"

if [[ $TYPE_ == 'Y' ]]; then
 
 while true; do
  echo "Syncing Data from IND12"
  rsync -avz --progress 10.23.227.62:$monthly_/NIFTY $local_m
  status1=$?
  [ $status1 -ne 0 ] && { echo "Sync Failed From IND12 NIFTY Retrying..."; sleep 1m; continue;  }

  rsync -avz --progress 10.23.227.62:$monthly_/BANKNIFTY $local_m
  status2=$?
  [ $status2 -ne 0 ] && { echo "Sync Failed From IND12 BANKNIFTY Retrying..."; sleep 1m; continue;  }

  rsync -avz --progress 10.23.227.62:$weekly_/NIFTY $local_w
  status1=$?
  [ $status1 -ne 0 ] && { echo "Sync Failed From IND12 WEEKLY NIFTY Retrying..."; sleep 1m; continue;  }

  rsync -avz --progress 10.23.227.62:$weekly_/BANKNIFTY $local_w
  status2=$?
  [ $status2 -ne 0 ] && { echo "Sync Failed From IND12 WEEKLY BANKNIFTY Retrying..."; sleep 1m; continue;  }

  echo "Syncing Worker"
  rsync -avz --progress /spare/local/MIDTERM_DATA/MediumTermOptions/BANKNIFTY dvctrader@52.90.0.239:/spare/local/MIDTERM_IDX/MediumTermOptions/
  status2=$?
  [ $status2 -ne 0 ] && { echo "Sync Failed To worker, Retrying..."; sleep 1m; continue;  }
  
  rsync -avz --progress /spare/local/MIDTERM_DATA/MediumTermOptions/NIFTY dvctrader@52.90.0.239:/spare/local/MIDTERM_IDX/MediumTermOptions/
  status2=$?
  [ $status2 -ne 0 ] && { echo "Sync Failed To worker, Retrying..."; sleep 1m; continue;  }

  rsync -avz --progress /spare/local/MIDTERM_DATA/MediumTermWeeklyOptions/BANKNIFTY dvctrader@52.90.0.239:/spare/local/MIDTERM_IDX/MediumTermWeeklyOptions/
  status2=$?
  [ $status2 -ne 0 ] && { echo "Sync Failed To worker, Retrying..."; sleep 1m; continue;  }
  
  rsync -avz --progress /spare/local/MIDTERM_DATA/MediumTermWeeklyOptions/NIFTY dvctrader@52.90.0.239:/spare/local/MIDTERM_IDX/MediumTermWeeklyOptions//
  status2=$?
  [ $status2 -ne 0 ] && { echo "Sync Failed To worker, Retrying..."; sleep 1m; continue;  } 
  break;
 done 
fi
echo "Calculating Geeks "

'''
echo "W BANKNIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYBANK /NAS2/spare/local/MIDTERM_IDX/MediumTermWeeklyOptions/BANKNIFTY $YYYYMMDD $YYYYMMDD W BANKNIFTY" >$file_mail 2>&1
echo "W NIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTY50 /NAS2/spare/local/MIDTERM_IDX/MediumTermWeeklyOptions/NIFTY $YYYYMMDD $YYYYMMDD W NIFTY" >>$file_mail 2>&1
'''

#HFT Data Greeks DB Gen 
'''echo "W_1 NIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTY50 /spare/local/INDEX_BARDATA/WEEKLYOPT/NIFTY $previous_day $previous_day W_1 NIFTY" >>$file_mail 2>&1
echo "M_1 NIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTY50 /spare/local/INDEX_BARDATA/MONTHLYOPT/NIFTY $previous_day $previous_day M_1 NIFTY" >>$file_mail 2>&1
echo "W_1 BANKNIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYBANK /spare/local/INDEX_BARDATA/WEEKLYOPT/BANKNIFTY $previous_day $previous_day W_1 BANKNIFTY" >>$file_mail 2>&1
echo "M_1 BANKNIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYBANK /spare/local/INDEX_BARDATA/MONTHLYOPT/BANKNIFTY $previous_day $previous_day M_1 BANKNIFTY" >>$file_mail 2>&1
'''

#HFT Data Greeks DB Gen 
echo "W_ALL NIFTY"
echo "W_ALL NIFTY" >>$file_mail 2>&1
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTY50 /spare/local/INDEX_BARDATA/WEEKLYOPT/NIFTY $previous_day $previous_day W_ALL NIFTY" >>$file_mail 2>&1
echo "M_ALL NIFTY"
echo "M_ALL NIFTY" >>$file_mail 2>&1
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTY50 /spare/local/INDEX_BARDATA/MONTHLYOPT/NIFTY $previous_day $previous_day M_ALL NIFTY" >>$file_mail 2>&1
echo "W_ALL BANKNIFTY"
echo "W_ALL BANKNIFTY"  >>$file_mail 2>&1
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYBANK /spare/local/INDEX_BARDATA/WEEKLYOPT/BANKNIFTY $previous_day $previous_day W_ALL BANKNIFTY" >>$file_mail 2>&1
echo "M_ALL BANKNIFTY"
echo "M_ALL BANKNIFTY"  >>$file_mail 2>&1
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYBANK /spare/local/INDEX_BARDATA/MONTHLYOPT/BANKNIFTY $previous_day $previous_day M_ALL BANKNIFTY" >>$file_mail 2>&1

file_mail_fin="/tmp/mail_for_the_Geeks_calucualtion_worker_finnifty"
echo "W_ALL FINNIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYFINSERVICE /spare/local/INDEX_BARDATA_TUES/WEEKLYOPT/FINNIFTY $previous_day $previous_day W_ALL FINNIFTY" >$file_mail_fin 2>&1
tail $file_mail_fin >>$file_mail

echo "M_ALL  FINNIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYFINSERVICE /spare/local/INDEX_BARDATA_TUES/MONTHLYOPT/FINNIFTY $previous_day $previous_day M_ALL FINNIFTY" >$file_mail_fin 2>&1
tail $file_mail_fin >>$file_mail
echo "DB Update Done" >>$file_mail 2>&1

# Midterm Data Greeks DB Gen
'''echo "M NIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTY50 /NAS2/spare/local/MIDTERM_IDX/MediumTermOptions/NIFTY $YYYYMMDD $YYYYMMDD M NIFTY" >>$file_mail 2>&1
echo "M BANKNIFTY"
ssh dvctrader@52.90.0.239 "/home/dvctrader/venv/bin/python3 /home/dvctrader/stable_exec/scripts/optionGreeksDBGen.py /spare/local/BarData_SPOT/NIFTYBANK /NAS2/spare/local/MIDTERM_IDX/MediumTermOptions/BANKNIFTY $YYYYMMDD $YYYYMMDD M BANKNIFTY" >>$file_mail 2>&1
'''

echo "STRADDLE 1"
#ssh dvctrader@52.90.0.239 "/home/dvctrader/stable_exec/scripts/straddle_python.sh $previous_day" >>$file_mail 2>&1
ssh dvctrader@52.90.0.239 "/home/dvctrader/stable_exec/scripts/straddle_python_new.sh $previous_day" >>$file_mail 2>&1

echo "Long Straddle" >>$file_mail 2>&1
echo "LONG STRADDLE 2"
ssh dvctrader@52.90.0.239 "/home/dvctrader/stable_exec/scripts/longcal_straddle_python.sh $previous_day" >> $file_mail 2>&1




cat $file_mail | mailx -s "Greek Calculated On Worker for ${YYYYMMDD}" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in rahul.yadav@tworoads-trading.co.in nishit.bhandari@tworoads.co.in infra_alerts@tworoads-trading.co.in arpit.agarwal@tworoads-trading.co.in ;
