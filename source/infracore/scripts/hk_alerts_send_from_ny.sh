

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

while true
do 

  scp dvcinfra@10.152.224.146:/spare/local/files/hk_data_daemon_alert_file.dat /tmp/hk_data_daemon_alert_file.dat >/dev/null 2>/dev/null 

  if [ -f /tmp/hk_data_daemon_alert_file.dat ] 
  then 

     /bin/mail -s "HKDataDaemon" -r "remotealert@ny11" "nseall@tworoads.co.in" < /tmp/hk_data_daemon_alert_file.dat 
     $HOME/infracore/scripts/sendAlert.sh `cat /tmp/hk_data_daemon_alert_file.dat`

     ssh dvcinfra@10.152.224.146 'rm -rf /spare/local/files/hk_data_daemon_alert_file.dat' ;

     rm -rf /tmp/hk_data_daemon_alert_file.dat 

  fi 

  scp dvcinfra@10.134.210.184:/spare/local/files/hk_data_daemon_alert_file.dat /tmp/hk_data_daemon_alert_file.dat >/dev/null 2>/dev/null 

  if [ -f /tmp/hk_data_daemon_alert_file.dat ] 
  then 

     /bin/mail -s "TOKDataDaemon" -r "remotealert@ny11" "nseall@tworoads.co.in" < /tmp/hk_data_daemon_alert_file.dat 
     $HOME/infracore/scripts/sendAlert.sh `cat /tmp/hk_data_daemon_alert_file.dat`

     ssh dvcinfra@10.134.210.184 'rm -rf /spare/local/files/hk_data_daemon_alert_file.dat' ;

     rm -rf /tmp/hk_data_daemon_alert_file.dat 

  fi 

  sleep 300 ;

done 

