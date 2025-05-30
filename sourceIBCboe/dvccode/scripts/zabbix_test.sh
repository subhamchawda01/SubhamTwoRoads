#! /bin/bash
USAGE="$0 input_annual_holiday_file active_since active_till year"

if [ $# -ne 4 ] ;
then
    echo -e $USAGE;
    exit;
fi

input_annual_holiday_file=$1;
active_since=$2;
active_till=$3;
year=$4;
active_since="`date +"%s" --date=$active_since`";
active_till="`date +"%s" --date=$active_till`";

for f in `cat $input_annual_holiday_file | tr ' ' '~'`; 
do 
	dates=();
	dates+="$(echo $f |  awk -F"~" '{print $6}' | awk -F"," '{for(i=1;i<=NF;i++) {print $i;}}')"; 
	groups=();
	groups+="$(echo $f |  awk -F"~" '{print $2}' | awk -F"," '{for(i=1;i<=NF;i++) {print $i;}}')"; 
	hosts=();
	hosts+="$(echo $f |  awk -F"~" '{print $3}' | awk -F"," '{for(i=1;i<=NF;i++) {print $i;}}')"; 
	exchange="$(echo $f |  awk -F"~" '{print $1}')";
	start_time="$(echo $f |  awk -F"~" '{print $4}')";
	period="$(echo $f |  awk -F"~" '{print $5}')";
    start_time_="43200"; 
	echo $period; echo $start_time;

	echo $exchange;
    `echo "USE zabbix_dumb_db; select MAX(maintenanceid)  as id into @mid from maintenances; \
	     INSERT INTO maintenances SET maintenanceid=IF(@mid IS NULL, @mid := 1, @mid :=1 + @mid), description='abc';\
	     UPDATE maintenances SET name='$year $exchange Exchange Holidays', active_since=$active_since,active_till=$active_till,maintenance_type=1,description='Maintenance of $exchange for $year holidays' \
	     where maintenanceid=@mid " | mysql -u root` ; 

	for group in `echo ${groups[*]}`
	do
		echo $group;
		`echo "USE zabbix_dumb_db; select groupid as id into @gid from groups where name like '$group'; \
		 select MAX(maintenanceid) as id into @mid from maintenances; \
		 select MAX(maintenance_groupid) as id into @mgid from maintenances_groups; \
		 INSERT INTO maintenances_groups SET maintenance_groupid=IF(@mgid IS NULL, @mgid := 1, @mgid :=1 + @mgid), maintenanceid=@mid, groupid=IF(@gid IS NULL, @gid := 1, @gid :=@gid); \
		 "| mysql -u root`;
	done     

	for host in `echo ${hosts[*]}`
	do
		echo $host;
		`echo "USE zabbix_dumb_db; select hostid as id into @hid from hosts where name like '$host'; \
		 select MAX(maintenanceid) as id into @mid from maintenances; \
		 select MAX(maintenance_hostid) as id into @mhid from maintenances_hosts; \
		 INSERT INTO maintenances_hosts SET maintenance_hostid=IF(@mhid IS NULL, @mhid := 1, @mhid :=1 + @mhid), maintenanceid=@mid, hostid=IF(@hid IS NULL, @hid := 1, @hid :=@hid); \
		 "| mysql -u root`;
	done

	for date in `echo ${dates[*]}`
	do
		echo $date;
		
		date_sec="`date +"%s" --date=$date`";
        date_sec=$(($date_sec+$start_time));

		`echo "USE zabbix_dumb_db; select MAX(timeperiodid) as id into @tid from timeperiods; \
		 INSERT INTO timeperiods SET timeperiodid=IF(@tid IS NULL, @tid := 1, @tid :=1 + @tid);  \
		 UPDATE timeperiods SET timeperiod_type=0, every=1, month=0, dayofweek=0, day=1, start_time=$start_time_, period=$period, start_date=$date_sec \
		 where timeperiodid=@tid; \
		 select MAX(maintenanceid) as id into @mid from maintenances; \
		 select MAX(maintenance_timeperiodid) as id into @mtid from maintenances_windows; \
		 INSERT INTO maintenances_windows SET maintenance_timeperiodid=IF(@mtid IS NULL, @mtid := 1, @mtid :=1 + @mtid), maintenanceid=@mid, timeperiodid=@tid; \
		 " | mysql -u root`;
	done

done 
