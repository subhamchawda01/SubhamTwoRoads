curr_time=`date +"%s"` ; 
remove_upto=`echo $curr_time | awk '{print $1-60}'` ; 
cnt=`echo "delete from history_text where clock<$remove_upto and value='' and itemid in (select itemid from items where name like 'QueryStatus')" | mysql zabbix -uroot` ; 
