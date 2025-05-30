#!/bin/bash
CONFIG_DIR="/home/pengine/prod/live_configs"
SMART_DAEMON_EXEC="/home/pengine/prod/live_scripts/SmartDaemonController.sh"
APPEND_FILE="/tmp/log_after_append"
declare -a user_id
declare -a user_pass
declare -a user_did_login=(false)
declare -a user_no
no_of_user=0


print_usage_and_exit () {
    echo "$0 DIR(MSF05) KEEP/CLEAR" ;
    exit ;
}


ors_down_user_logged_in(){
	
	echo "" | mailx -s "ERROR::ORS DOWN AND USER LOGGED IN" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in ; exit; 
}


load_login_config(){
  echo "LOADING CONFIG FILE"
  [[ -s $CONFIG_FILE ]] || { echo "" | mailx -s "ERROR::CONFIG FILE NOT PRESENT FOR ORS LOGIN" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in ; exit; }
  user_no=(`grep "Multisessions" $CONFIG_FILE | cut -d' ' -f2 `)
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


# unused
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

# unused
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


restart_ors(){
	
	echo "Restarting the ORS\n"
	update_and_stop_ors;
        start_ors_bk;
        sleep 2m
	file_after_last_apend;
        if ! grep -aq "LOGIN_USERID -> ${user_id[0]}" $APPEND_FILE ; then
                echo "" | mailx -s "ERROR::ORS RESTARED AND USER NOT LOGGED IN" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in ; exit;
	
	else
		checkaddts_done;
        fi

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
#	echo "" | mailx -s "ORS Logged in & Addts done on ${HOSTNAME}: $((log_count-100))" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
	echo "ADDTS done. Log: $((log_count-100)) __ ADDTSFile: $file_count"
  else
#	echo "" | mailx -s "ERROR:ORS Logged in But Addts Problem on ${HOSTNAME}: $((log_count-100)). ADDTSFile:$file_count" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in
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

check_login_error(){
  echo "Checking Login Errors"
   if grep -aq "Login Failed. Error Code:" $APPEND_FILE; then
       echo "EXCHANGE ERROR: ERROR in login Exiting..."
       echo "" | mailx -s "ERROR: EXCHANGE ERROR" -r "${HOSTNAME}-${USER}<tarun.joshi@tworoads-trading.co.in>" tarun.joshi@tworoads-trading.co.in sanjeev.kumar@tworoads-trading.co.in subham.chawda@tworoads-trading.co.in raghunandan.sharma@tworoads-trading.co.in 
       exit
   fi
}

# unused
wait_for_ors(){
  echo "wait for ors to exit"
  pid_ors=`ps aux | grep "/home/pengine/prod/live_scripts/SmartDaemonController.sh ORS" | grep -v "grep" | awk -F ' +' '{print $2}'`
  while [ ! -z $pid_ors ] ;do
    sleep 3s;
    pid_ors=`ps aux | grep "/home/pengine/prod/live_scripts/SmartDaemonController.sh ORS" | grep -v "grep" | awk -F ' +' '{print $2}'`
  done
  echo "Smartdaemon ors exited"
}

#Main Function

#Checking the count of input arguments
if [ $# -ne 2 ] ; then
  echo "Called As : " $* ;
  print_usage_and_exit ;
fi

#Checking wheter today is a holiday or not
today_date=`date +\%Y\%m\%d`
echo "Running for the day $today_date..."
is_holiday=`/home/pengine/prod/live_execs/holiday_manager EXCHANGE NSE $today_date T`
if [ $is_holiday = "1" ] ; then
   echo "NSE Holiday. Exiting...";
   exit;
fi

#Initalizing the profile & pos variables
PROFILE=$1
POS=$2
CONFIG_FILE="/home/pengine/prod/live_configs/common_"$PROFILE"_ors.cfg" ;
TEMP_CONFIG="/tmp/common_"$PROFILE"_ors.cfg"

#Create backup of Config file
mkdir -p "/home/dvcinfra/important/CONFIG_BACKUP/"
cp $CONFIG_FILE "/home/dvcinfra/important/CONFIG_BACKUP/common_${PROFILE}_ors.cfg_bkp_$today_date"
# remove older files
find /home/dvcinfra/important/CONFIG_BACKUP/ -type f -mtime +7 -execdir rm -- '{}' \;
echo "FIND complete "
#Selecting the exchange server from which ORS will try login
EXCH=`grep "Exchange " $CONFIG_FILE | awk '{print $2}'`;
LOG_FILE="/spare/local/ORSlogs/$EXCH/$PROFILE/log.$today_date"

echo "load_login_config"
load_login_config;

echo "check_ors_status "
check_ors_status;

#If ORS is not running
if ! $ors_status ; then
	#Check if user is logged in
	echo "ORS is not running\n"
	file_after_last_apend;
        if grep -aq "LOGIN_USERID -> ${user_id[0]}" $APPEND_FILE ; then
		echo "ORS is not running and user is loggedIn\n"
		ors_down_user_logged_in;
	fi
	#Checking for EXCHANGE Error i.e Login error
	echo "check_login_error"
	check_login_error;

	#ORS is down && No user logged in && No Exchange error
	echo "restart_ors"
	restart_ors;
else
	file_after_last_apend;
	#Checking for EXCHANGE Error i.e Login error
	echo "ORS is already running\n"
        check_login_error;

	#ORS is up && No user logged in && No Exchange error
        #Check if not logged in
        if ! grep -aq "LOGIN_USERID -> ${user_id[0]}" $APPEND_FILE ; then
                echo "ORS is running and user not logged in\n"
               	restart_ors;
        fi

	#ORS is up && User is logged in
fi
