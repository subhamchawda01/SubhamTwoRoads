#!/bin/bash
CONFIG_DIR="/home/pengine/prod/live_configs"
SMART_DAEMON_EXEC="/home/pengine/prod/live_scripts/SmartDaemonController.sh"
APPEND_FILE="/tmp/log_after_append"
declare -a user_id
declare -a user_pass
declare -a user_did_login=(false false)
declare -a user_no
no_of_user=0

print_usage_and_exit () {
    echo "$0 DIR(MSF05) KEEP/CLEAR" ;
    exit ;
}

#load_config
load_login_config(){
  echo "LOADING CONFIG FILE"
  [[ -s $CONFIG_FILE ]] || { echo "" | mailx -s "ERROR::CONFIG FILE NOT PRESENT FOR ORS LOGIN" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in ; exit; }
  user_no=(`grep "Multisessions" $CONFIG_FILE | cut -d' ' -f2| cut -d',' -f1- --output-delimiter=' '`)
  i=0
  no_of_user=${#user_no[@]}
  for user in ${user_no[@]} ;do 
  	user_id[$i]=`grep "\bUserName-$user\b" $CONFIG_FILE |cut -d' ' -f2`
        user_pass[$i]=`grep "\bPassword-$user\b" $CONFIG_FILE |cut -d' ' -f2`
	echo "USER$i: ${user_id[$i]}  Pass: ${user_pass[$i]}"
 	i=$((i+1));
  done
  pass_date=`grep "\bPasswordChangeDate-1\b" $CONFIG_FILE| cut -d' ' -f2`
  pass_change_date=`date --date="$pass_date +7 days" "+%Y%m%d"`
  change_pass=false
  [[ $today_date < $pass_change_date ]] || change_pass=true;
  echo "No of User on Machine: $no_of_user"
  echo "PASSWORD change date: $pass_change_date"
}

check_ors_status(){
  if [[ `ps aux | grep cme| grep -v "grep" |wc -l` < "1" ]]; then
  	echo "ERROR::ORS NOT RUNNING ON THIS SERVER"
  	ors_status=false
  else
  	echo "ORS RUNNING"
        ors_status=true
  fi
}

#check no of logins
check_login_count(){
  echo "CHECKING USER LOGIN..."
  file_after_last_apend
  logged_count=`grep -a "LOGIN_USERID ->" $APPEND_FILE | wc -l`
  echo "Logged_user $logged_count"
  if [[ $no_of_user -eq $logged_count ]];then
  	echo "ALL USER lOGGED IN $no_of_user";
#	while Loop will not run
	run_and_login=false
  else
      echo "Not all User Logged In"
        run_and_login=true
  fi
}

start_ors(){
  echo "STARTING ORS"
  $SMART_DAEMON_EXEC ORS $PROFILE START $POS >/dev/null 2>/dev/null
}

start_ors_bk(){
  echo "STARTING ORS in background"
  $SMART_DAEMON_EXEC ORS $PROFILE START $POS >/dev/null 2>/dev/null &
  pid_ors=$!
}

stop_ors(){  
  echo "STOPING ORS"
  $SMART_DAEMON_EXEC ORS $PROFILE STOP $POS >/dev/null 2>/dev/null
  #empty cerror and cout file
  >"/spare/local/ORSlogs/$EXCH/$PROFILE/cme_ilink_ors.COUT.CERR$today_date"
}

file_after_last_apend(){
  line_no=`grep -nar "Opened ORSLog in Append" $LOG_FILE | tail -1 | cut -d':' -f1`
  tail -n +$line_no $LOG_FILE > $APPEND_FILE
}

checkaddts_done(){
  echo "Checking Addts Done"
  ADDTS_FILE="/home/pengine/prod/live_configs/${HOSTNAME}_addts.cfg"
  file_count=`cat $ADDTS_FILE | wc -l`
  file_after_last_apend;
  log_count=`grep -a "ADDTRADING" $APPEND_FILE|wc -l`
# Add NO TO ADDTS FOR RACE Condition
  log_count=$((log_count+100))   
  if [[ $log_count -ge $file_count ]]; then
#	echo "" | mailx -s "ORS Logged in & Addts done on ${HOSTNAME}: $((log_count-100))" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
	echo "ADDTS done. Log: $((log_count-100)) __ ADDTSFile: $file_count"
  else
#	echo "" | mailx -s "ERROR:ORS Logged in But Addts Problem on ${HOSTNAME}: $((log_count-100)). ADDTSFile:$file_count" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
	echo "Addts Failed: $((log_count-100))"
  fi
}

update_and_stop_ors(){
  stop_ors;
  if [[ $change_pass == true ]]; then
	echo "Updating File";
	i=0
  	for user in ${user_id[@]};do
		if grep -aq "LOGIN_USERID -> $user" $LOG_FILE ; then
			echo "USER ALREADY LOGGED IN $user"
			user_did_login[$i]=true
		fi
		i=$((i+1));
        done
	i=0
	`cp $CONFIG_FILE $TEMP_CONFIG`
	prev_date=$pass_date
        today_day=`date "+%d"`
        today_month=`date "+%b"`
        today_year=`date "+%y"`
	while [ $i -lt $no_of_user ];do
		if [[ ${user_did_login[$i]} == false ]]; then 
			sed -i "s/PasswordChangeDate-$((i+1)) ${today_date}/PasswordChangeDate-$((i+1)) ${prev_date}/g" $TEMP_CONFIG
                        sed -i "s/Password-$((i+1)) ${today_day}${today_month}@${today_year}/Password-$((i+1)) ${user_pass[$i]}/g" $TEMP_CONFIG
                else 
                        sed -i "s/PasswordChangeDate-$((i+1)) ${prev_date}/PasswordChangeDate-$((i+1)) ${today_date}/g" $TEMP_CONFIG
                        sed -i "s/Password-$((i+1)) ${user_pass[$i]}/Password-$((i+1)) ${today_day}${today_month}@${today_year}/g" $TEMP_CONFIG
		fi
		i=$((i+1));
	done
	`cp $TEMP_CONFIG $CONFIG_FILE`

  fi
}

check_2nd_login(){
  user2login=false
  echo "Checking 2nd User Login"
  if [[ $no_of_user -gt 1 ]];then 
	if grep -aq "LOGIN_USERID -> ${user_id[1]}" $APPEND_FILE; then
		echo "Second User Logged in. Wait for 3 mins..."
		user2login=true
		sleep 3m
	fi
  fi
  if ! $user2login ;then
	countLogin=`grep -a "LOGIN_USERID -> " $APPEND_FILE | wc -l`
	countLogout=`grep -a "Logout Received" $APPEND_FILE | wc -l`
	if [[ $countLogin -ne $countLogout ]];then
		echo "Logout Not Received. Wait for 3 mins..."
		sleep 3m
	fi
  fi
}

check_login_error(){
  echo "Checking Login Errors"
   if grep -aq "Login Failed. Error Code:" $APPEND_FILE; then
       echo "ERROR in login Exiting..."
       echo "" | mailx -s "ERROR:ORS Logged IN ERROR. Check Manually" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in hardik.dhakate@tworoads-trading.co.in
       exit
   fi
}

wait_for_ors(){
  echo "wait for ors to exit"
  pid_ors=`ps aux | grep "/home/pengine/prod/live_scripts/SmartDaemonController.sh ORS" | grep -v "grep" | awk -F ' +' '{print $2}'`
  while [ ! -z $pid_ors ] ;do
    sleep 3s;
    pid_ors=`ps aux | grep "/home/pengine/prod/live_scripts/SmartDaemonController.sh ORS" | grep -v "grep" | awk -F ' +' '{print $2}'`
  done
  echo "Smartdaemon ors exited"
}

#main
if [ $# -ne 2 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi
today_date=`date +\%Y\%m\%d`
echo "Running for the day $today_date..."
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi
PROFILE=$1
POS=$2
CONFIG_FILE="/home/pengine/prod/live_configs/common_"$PROFILE"_ors.cfg" ;
TEMP_CONFIG="/tmp/common_"$PROFILE"_ors.cfg"
#create backup of config
mkdir -p "/home/dvcinfra/important/CONFIG_BACKUP/"
cp $CONFIG_FILE "/home/dvcinfra/important/CONFIG_BACKUP/common_${PROFILE}_ors.cfg_bkp_$today_date"
# remove older files
find /home/dvcinfra/important/CONFIG_BACKUP/ -type f -mtime +7 -execdir rm -- '{}' \;

EXCH=`grep "Exchange " $CONFIG_FILE | awk '{print $2}'`;
LOG_FILE="/spare/local/ORSlogs/$EXCH/$PROFILE/log.$today_date"
run_and_login=true
ors_status=true

load_login_config;
check_ors_status;
if $ors_status ; then
  wait_for_ors;
  check_login_count;
  if $run_and_login ;then 
	update_and_stop_ors;
	file_after_last_apend;
        check_login_error;
	check_2nd_login;
  fi
fi
#Loop till all the user are connected to the exchange
while $run_and_login; do
  stop_ors;
  start_ors_bk;
  sleep 5s
  i=0
  echo "Waiting For Connect..."
  while [[ $i -lt 7 ]];do
	file_after_last_apend;
	if grep -aq "Connected" $APPEND_FILE ;then
		break;
	fi
	sleep 3s
	i=$((i+1));
  done
  #check if ORS is connected to the NSE
  file_after_last_apend;
  if ! grep -aq "Connected" $APPEND_FILE ;then
	echo "ORS not Connected. Restarting..."
	update_and_stop_ors;
	file_after_last_apend;
        check_login_error;
	check_2nd_login;	
#	continue	
  fi
  #Check User logged In
  echo "Connected."
  i=0
  for user in ${user_id[@]} ;do  
  	echo "Checking loggin for $i User: $user"
  	sleep 6s
  	file_after_last_apend;
  	if grep -aq "LOGIN_USERID -> ${user}" $APPEND_FILE ; then
		echo "User $user Logged In"
  		check_login_count;
  	else
                echo "User $user Failed To Log In"
		update_and_stop_ors;
                echo "pid: $pid_ors"
                kill $pid_ors
		file_after_last_apend;
                check_login_error;
		check_2nd_login;
                break;
  	fi
	i=$((i+1));
  done 

  [[ $no_of_user == $((i)) ]] || continue;
  echo "Wait for addts to complete..."
  wait $pid_ors;
# if failed doing addts
  check_ors_status;
  [[ $ors_status == false ]] &&  run_and_login=true;
done

# check and send mail about succes ors
file_after_last_apend;
checkaddts_done;

