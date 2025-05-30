#!/bin/bash

LOCK_FILE=$HOME/AWSScheduler/.lock_cleaner_lockfile
if [ -f $LOCK_FILE ]
then
  echo "Lock-Cleaner Lock file $LOCK_FILE exists. Can't start process";
  exit 1;
fi
touch $LOCK_FILE
trap "rm -f $LOCK_FILE" EXIT

uid=`date +%N`;
tmpfile=tmp_clean_"$uid";
AWS_DIR=$HOME/AWSScheduler;
instance_cores_file="/mnt/sdf/JOBS/all_instances.txt";

ls /mnt/sdf/locks/call_run_sim_overnight_* > prev_locks;
ls /mnt/sdf/locks/generate_multiple_indicator_stats_* >> prev_locks;

> $tmpfile;

for instance in `cat $instance_cores_file | grep nil | awk '{print $NF}'`; do
  ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=10 $instance "
    ps -ef | grep call_run_sim_overnight | grep -v \"grep call_run_sim_overnight\"; 
    ps -ef | grep gen_ind_stats_from_config.sh | grep -v \"grep gen_ind_stats_from_config.sh\";
  " >> $tmpfile 2>/dev/null;
done

cat $tmpfile | grep "call_run_sim_overnight_perdir_longer.pl" | awk '{print "/mnt/sdf/locks/call_run_sim_overnight_longer_"$(NF-2)"_"$(NF-1)"_"$NF".lock"}' > curr_locks;
cat $tmpfile | grep "call_run_sim_overnight_perdir.pl" | awk '{if(NF==10){$11=5;}} {if(NF>=11){print "/mnt/sdf/locks/call_run_sim_overnight_"$10"_"$11"_"$12".lock"}}' >> curr_locks;
cat $tmpfile | grep "call_run_sim_overnight_perdir_longer_staged.pl" | awk '{print "/mnt/sdf/locks/call_run_sim_overnight_longer_staged_"$(NF-2)"_"$(NF-1)"_"$NF".lock"}' >> curr_locks;
cat $tmpfile | grep "call_run_sim_overnight_perdir_staged.pl" | awk '{if(NF==10){$11=5;}} {if(NF>=11){print "/mnt/sdf/locks/call_run_sim_overnight_staged_"$10"_"$11"_"$12".lock"}}' >> curr_locks;
for i in `cat $tmpfile | grep "gen_ind_stats_from_config.sh" | awk '{print $10}'`; do awk '{if($1=="SELF"){shc=$2;} if($1=="TIMEPERIODSTRING"){tp=$2;} if($1=="DATAGEN_BASE_FUT_PAIR"){pr=$2"_"$3;}} END{print "/mnt/sdf/locks/generate_multiple_indicator_stats_"shc"_"tp"_"pr".lock";}' $i; done >> curr_locks;

#remove stale locks
for i in `cat prev_locks`; do if [ `grep $i curr_locks | wc -l` -eq 0 ] ; then echo REMOVING: $i; rm $i; fi; done;

#create missing locks
for i in `cat curr_locks`; do if [ ! -e $i  ] ; then echo CREATING: $i; touch $i; fi; done;

rm prev_locks curr_locks $tmpfile;

#clean virtual locks
if [ -d /mnt/sdf/locks ] ; then
find /mnt/sdf/locks/*_virtuallock_dont_delete_this -type f -mtime +1 -exec rm -f {} \;
fi

rm -f $LOCK_FILE
