#!/bin/bash
#Dependencies
PEXEC_BIN="/home/pengine/prod/live_execs/"
PENGINE_CONFIG="/home/pengine/prod/live_configs/"
SERVER_LIST_FILE="/home/pengine/prod/live_configs/all_servers.cfg"

#Config Directories to compare

REPO_PATH="/home/pengine/codebase/dvccode/configs/"

#Files and directories used for comparisons and script tasks
DIFF_REPORTER_DIR="/home/pengine/trash/config_diff_reporter/"
TEMP_DIR="${DIFF_REPORTER_DIR}remote_cfg/"
REPO_MISSING_LIST_TXT="${DIFF_REPORTER_DIR}repo_missng.txt"
PROD_MISSING_LIST_TXT="${DIFF_REPORTER_DIR}prod_missing.txt"
TMP_FILE_TXT="${DIFF_REPORTER_DIR}tmp.txt"
DIFF_LIST_TXT="${DIFF_REPORTER_DIR}diff_file.txt"

SLACK_CHANNEL="notification_monitor"

#Setup Directories if not already present
mkdir -p $DIFF_REPORTER_DIR
mkdir -p $TEMP_DIR
#Create and clear file content
rm $TEMP_DIR*
touch $REPO_MISSING_LIST_TXT; echo "Files missing in repo"> $REPO_MISSING_LIST_TXT
touch $PROD_MISSING_LIST_TXT; echo "Files missing in prod"> $PROD_MISSING_LIST_TXT
touch $DIFF_LIST_TXT; echo "Files with Differences"> $DIFF_LIST_TXT

#Update the local git repo
cd $REPO_PATH
cd ..
git reset --hard 
git checkout master
git pull

#Command for fetching config files list from prod-servers
get_file_list="ls -lrt $PENGINE_CONFIG | awk '{print \$9;}'| grep \`hostname\`"

#Initialize counter variables to keep track of files on all servers
repo_missing_file_cnt=0;
prod_missing_file_cnt=0;
diff_file_cnt=0;

#Main Loop iterating over all prod servers
for server in `cat $SERVER_LIST_FILE | grep -v "#"`
do
  #Check if server is reachable
  status=$(ssh dvcinfra@$server echo ok 2>/dev/null </dev/null)
  if [ "$status" == "ok" ]
    then
    #Get list of files present on prod server and config files with the hostname on dvccode
    host_name=`ssh dvcinfra@$server "hostname"`
    echo $host_name;
    prod_server_file_list=`ssh dvcinfra@$server "$get_file_list"`
    prod_server_file_cnt=`echo $prod_server_file_list| wc -w`
    echo "Pconfig File count: "$prod_server_file_cnt
    repo_file_list=`ls -lrt $REPO_PATH | grep $host_name | awk '{print $NF}'`
    repo_file_cnt=`echo $repo_file_list| wc -w`
    echo "Repo File count: "$repo_file_cnt

    #Get List of configs present on prod server but not updated to git
    curr_repo_missing_file_cnt=0;
    > $TMP_FILE_TXT
    for file in $prod_server_file_list
    do
      is_present=`echo $repo_file_list| grep -w $file| wc -l`
      if [ $is_present -eq 0 ]
       then
       curr_repo_missing_file_cnt=$(($curr_repo_missing_file_cnt + 1))
       echo $file >> $TMP_FILE_TXT;
      fi
    done

    if [ $curr_repo_missing_file_cnt -ne 0 ]
      then
      repo_missing_file_cnt=$(($repo_missing_file_cnt+ $curr_repo_missing_file_cnt))
      echo $server >> $REPO_MISSING_LIST_TXT
      cat $TMP_FILE_TXT >> $REPO_MISSING_LIST_TXT;
      echo "\n" >>$REPO_MISSING_LIST_TXT

    fi

  #Get list of files on git but not present on prod server
    curr_prod_missing_file_cnt=0;
    > $TMP_FILE_TXT
    for file in $repo_file_list
    do
      is_present=`echo $prod_server_file_list| grep -w $file| wc -l`
      if [ $is_present -eq 0 ]
       then
       echo $file >> $TMP_FILE_TXT;
       curr_prod_missing_file_cnt=$(($curr_prod_missing_file_cnt + 1))
      fi
    done

    if [ $curr_prod_missing_file_cnt -ne 0 ]
      then
      echo $server >> $PROD_MISSING_LIST_TXT;
      cat $TMP_FILE_TXT >> $PROD_MISSING_LIST_TXT;
      prod_missing_file_cnt=$(( $prod_missing_file_cnt + $curr_prod_missing_file_cnt ))
      echo "\n" >>$PROD_MISSING_LIST_TXT
    fi

  #Get list of configs present on both but are different
    curr_diff_file_cnt=0;
    > $TMP_FILE_TXT
    for file in $prod_server_file_list
    do
      is_present=`echo $repo_file_list| grep -w $file| wc -l`
      if [ $is_present -ne 0 ]
       then
        scp dvcinfra@${server}:${PENGINE_CONFIG}$file $TEMP_DIR
        Diff=`diff ${TEMP_DIR}$file ${REPO_PATH}$file | wc -l`
        if [ $Diff -ne 0 ]
          then
          echo "$file" >> $TMP_FILE_TXT
          curr_diff_file_cnt=$(($curr_diff_file_cnt + 1))
        fi
      fi
    done

    if [ $curr_diff_file_cnt -ne 0 ]
      then
      echo $server >> $DIFF_LIST_TXT;
      cat $TMP_FILE_TXT >> $DIFF_LIST_TXT;
      diff_file_cnt=$(( $diff_file_cnt + $curr_diff_file_cnt ))
      echo "\n" >> $DIFF_LIST_TXT
    fi

    echo " "
  else
      $PEXEC_BIN/send_slack_notification $SLACK_CHANNEL "DATA" "$server not reachable."    
   fi
done

#Send Slack Alerts if there are difference reported
if [ $repo_missing_file_cnt -ne 0 ]
  then
  $PEXEC_BIN/send_slack_notification $SLACK_CHANNEL FILE $REPO_MISSING_LIST_TXT
fi

if [ $prod_missing_file_cnt -ne 0 ]
  then
  $PEXEC_BIN/send_slack_notification $SLACK_CHANNEL FILE $PROD_MISSING_LIST_TXT
fi

if [ $diff_file_cnt -ne 0 ]
  then
  $PEXEC_BIN/send_slack_notification $SLACK_CHANNEL FILE $DIFF_LIST_TXT
fi

if [ $diff_file_cnt -eq 0 -a $prod_missing_file_cnt -eq 0 -a $repo_missing_file_cnt -eq 0 ]
  then
  $PEXEC_BIN/send_slack_notification $SLACK_CHANNEL DATA "All config files in sync with repo"
fi

#Clean-up
rm ${TEMP_DIR}*
