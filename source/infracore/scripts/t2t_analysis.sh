#!/bin/bash

USAGE="$0 NUM_DAYS";
if [ $# -lt 1 ] ;
then
    echo $USAGE;
    exit;
fi

num_days=$1

cwriter_file="/tmp/temp_t2t_cwriter"
shm1_file="/tmp/temp_shm1"
tradeinit_file="/tmp/temp_t2t_tradeinit"
shm2_file="/tmp/temp_t2t_shm2"
ors_file="/tmp/temp_t2t_ors"
final_file="/tmp/temp_t2t_final"

get_mean(){ 
  file=$1
  n=`cat "$file" | wc -l`
  sum=`cat "$file" | /home/pengine/prod/live_scripts/sumcalc.pl`
  
 mean_output=`awk -v sum=$sum -v n=$n "BEGIN { if (n == 0 ){ print 0 } else { print sum/n } }"`
}

check_and_remove(){
  if [ -f $1 ]
  then 
    rm $1
  fi
}

check_for_invalid_stats(){
  mean_by_median=`echo $1" "$2 | awk '{print int($1/$2)}'`;
  is_stats_invalid=2
  if [ $mean_by_median -gt 50 ]
  then
    is_stats_invalid=1
  fi
}

get_stats(){
  cd /spare/local/logs/profilelogs
  offset=$1
  check_and_remove $cwriter_file
  check_and_remove $shm1_file
  check_and_remove $tradeinit_file
  check_and_remove $shm2_file
  check_and_remove $ors_file
  check_and_remove $final_file
  
  for f in `ls -lrt | grep runtime_profiler | grep -v debug | tail -$num_days | tr -s ' ' | cut -d' ' -f9` 
  do
    #check for zipped file and use appropriate cat and grep
    if [ -z "`gzip -t $f 2>&1`" ]
    then
       cat_type=zcat
       grep_type=zgrep
    else
       cat_type=cat
       grep_type=grep
    fi

    if [ `$grep_type Overall $f | wc -l` -eq 0 ]
    then
#echo "Skipping File $f"
       continue
    fi
  
    if [ `$grep_type Overall $f | tail -1 | cut -d' ' -f2` -eq 0 ]
    then 
#echo "Skipping File $f"
       continue
    fi
   
    t2t_overall_mean=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f42`
    t2t_overall_median=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f43` 
    check_for_invalid_stats $t2t_overall_mean $t2t_overall_median
    if [ $is_stats_invalid -eq 1 ]
    then 
#  echo "Skipping $f due to Invalid Stats"
      continue
    fi
  
    val=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f$offset`
    echo "$val" >> $cwriter_file
  
    val=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f$((offset+7))`
    echo "$val" >> $shm1_file
  
    val=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f$((offset+14))`
    echo "$val" >> $tradeinit_file
  
    val=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f$((offset+21))`
    echo "$val" >> $shm2_file
  
    val=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f$((offset+28))`
    echo "$val" >> $ors_file
  
    val=`$cat_type $f | grep Overall | tail -1 | tr -s ' ' | cut -d' ' -f$((offset+35))`
    echo "$val" >> $final_file
  done
 
  cwriter_stats=0
  if [ -f $cwriter_file  ]
  then 
    get_mean $cwriter_file
    cwriter_stats=$mean_output
  fi

  shm1_stats=0
  if [ -f $shm1_file ]
  then
    get_mean $shm1_file
    shm1_stats=$mean_output
  fi
  
  tradeinit_stats=0
  if [ -f $tradeinit_file ]
  then
    get_mean $tradeinit_file
    tradeinit_stats=$mean_output
  fi

  shm2_stats=0
  if [ -f $shm2_file ]
  then  
    get_mean $shm2_file
    shm2_stats=$mean_output
  fi
  
  ors_stats=0
  if [ -f $ors_file ]
  then 
    get_mean $ors_file
    ors_stats=$mean_output
  fi

  t2t_stats=0
  if [ -f $final_file ]
  then
    get_mean $final_file
    t2t_stats=$mean_output
  fi

}


get_stats 7
echo "Mean Stats: "$cwriter_stats" "$shm1_stats" "$tradeinit_stats" "$shm2_stats" "$ors_stats" "$t2t_stats

get_stats 8
echo "Median Stats: "$cwriter_stats" "$shm1_stats" "$tradeinit_stats" "$shm2_stats" "$ors_stats" "$t2t_stats

get_stats 9
echo "95P Stats: "$cwriter_stats" "$shm1_stats" "$tradeinit_stats" "$shm2_stats" "$ors_stats" "$t2t_stats

get_stats 10
echo "99P Stats: "$cwriter_stats" "$shm1_stats" "$tradeinit_stats" "$shm2_stats" "$ors_stats" "$t2t_stats

check_and_remove $cwriter_file
check_and_remove $shm1_file
check_and_remove $tradeinit_file
check_and_remove $shm2_file
check_and_remove $ors_file
check_and_remove $final_file
