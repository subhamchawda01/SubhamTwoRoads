#!/bin/bash

affinity_view_exec=/home/pengine/prod/live_execs/cpu_affinity_mgr ; 
affinity_tracking_html=/tmp/.affinity_tracking.html
affinity_monitor_file=/tmp/.affinity_monitoring.dat

>$affinity_tracking_html ; 

echo '<!DOCTYPE html><html><body>' >> $affinity_tracking_html
echo '<table border="1" style="width:100%" cellpadding="20"><tr><th align=center>COLOR</th><th align=center>MEANING</th></tr>' >> $affinity_tracking_html ;
echo '<tr><td align=center>WHITE</td><td>FREE_CORE</td></tr>' >> $affinity_tracking_html ; 
echo '<tr><td align=center>GREEN</td><td>CORE_IN_USE</td></tr>' >> $affinity_tracking_html ; 
echo '<tr><td align=center>GRAY</td><td>INIT_PROCESS_CORES</td></tr>' >> $affinity_tracking_html ; 
echo '<tr><td align=center>LIGHT_RED</td><td>MULTIPLE_PROCESS_ON_SAME_CORE( CAN BE CLUBBED STRATS AS WELL)</td></tr>' >> $affinity_tracking_html ; 
echo '<tr><td align=center>RED</td><td>PROCESS_RUNNIG_OFF_INIT_CORES</td></tr><tr></tr><tr></tr><tr></tr><tr></tr>' >> $affinity_tracking_html ; 

echo '<table border="1" style="width:100%" cellpadding="10"><tr><th align=center>CORE</th><th align=center>PID</th><th align=center>PROCESS_AFFINED</th></tr>' >> $affinity_tracking_html ;

color="#FFFFFF" ; 
init_cores=`$affinity_view_exec VIEW init 1 | awk -F"CORE #" '{print $2}' | xargs` ; 
total_cores=`cat /proc/cpuinfo | grep -w "processor" | wc -l`

for cores in `seq 0 $((total_cores-1))` ; do 

  is_this_init_core=0 ; 

  if [ `echo $init_cores | grep -w "$cores" | wc -l` -gt 0 ] ; then 
    is_this_init_core=1; 
    color="#A4A4A4" ; 
  fi   

  if [ `grep -w "$cores" /spare/local/files/affinity/affinity_pid_process.txt | wc -l` -gt 0 ] ; then 

    procs_on_a_core=0 ;
    running_pid="" ; 
    running_proc="" ;

    for lines in `grep -w "$cores" /spare/local/files/affinity/affinity_pid_process.txt | tr ' ' '~'` ; do 

      pid=`echo $lines | awk -F"~" '{print $2}'` ;
      procs=`echo $lines | awk -F"~" '{print $1}'` ; 

      if [ `ps -efL | grep "$pid" | grep -v grep | wc -l` -eq 0 ] ; then
        continue ; 
      fi 

      ((procs_on_a_core++)) ;

      running_pid=`echo $running_pid" "$pid` ;
      running_proc=`echo $running_proc" "$procs` ;

      if [ $is_this_init_core -eq 1 ] ; then 
        echo "Process $procs With PID $pid Is Running Off Init Cores -> $cores" >> $affinity_monitor_file ; 
      fi 

    done 

    print_core=`echo $cores` ;

    if [ $procs_on_a_core -eq 1 ] ; then 
      
       if [ $is_this_init_core -eq 1 ] ; then 
         color="#FE2E2E" ;
         print_core="INIT" ;
       else 
         color="#58FAAC" ; 
       fi 

       echo "<tr bgcolor="$color">" >> $affinity_tracking_html ; 
       echo "<td align=center>$print_core</td>" >> $affinity_tracking_html ; 
       echo "<td align=center>$pid</td>" >> $affinity_tracking_html ; 
       echo "<td align=center>$procs</td>" >> $affinity_tracking_html ; 
    else 

       if [ $is_this_init_core -eq 1 ] ; then 
         color="#FE2E2E" ;
       else 
         color="#F7819F" ; 
       fi 

       echo "<tr bgcolor="$color">" >> $affinity_tracking_html ; 
       echo "<td align=center>$print_core</td>" >> $affinity_tracking_html ; 
       echo "<td align=center>$running_pid</td>" >> $affinity_tracking_html ; 
       echo "<td align=center>$running_proc</td>" >> $affinity_tracking_html ; 
    fi 

  else 

   if [ $is_this_init_core -eq 1 ] ; then 
  
     color="#A4A4A4" ; 
     echo "<tr bgcolor="$color">" >> $affinity_tracking_html ; 
     echo "<td align=center>INIT</td>" >> $affinity_tracking_html ; 
     echo "<td align=center>NA</td>" >> $affinity_tracking_html ; 
     echo "<td align=center>NA</td>" >> $affinity_tracking_html ; 
         
   else 

     color="#FFFFFF" ; 
     echo "<tr bgcolor="$color">" >> $affinity_tracking_html ; 
     echo "<td align=center>$cores</td>" >> $affinity_tracking_html ; 
     echo "<td align=center>NA</td>" >> $affinity_tracking_html ; 
     echo "<td align=center>NA</td>" >> $affinity_tracking_html ; 
   
   fi 

  fi

done 

echo "</table></body></html>" >> $affinity_tracking_html ; 

cat $affinity_tracking_html ; 
