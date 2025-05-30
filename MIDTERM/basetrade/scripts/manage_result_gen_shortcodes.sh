USAGE="$0 0[add]/1[remove]/2[list] exchange shortcode"

if [ $# -lt 2 ]; then 
	echo "$USAGE"
	exit 1;
fi

action=$1; shift
exchange=$1; shift
shortcode=$1; 

if [ ! -e /media/disk-ephemeral2/command_files/procs_$exchange ]; then
	echo "Valid Exchanges: `find /media/disk-ephemeral2/command_files -maxdepth 1 -type f  | awk -F'/' '{print $NF}' | cut -d'_' -f2 | sort`"
	exit 1;
fi

if [ "$action" == "0" ] || [ "$action" == "1" ]; then
	if [ "$shortcode" == "" ]; then
		echo "Enter a shortcode!"
		exit 1;
	fi
fi

if [ "$action" == "0" ]; then
	is_present=`cat /media/disk-ephemeral2/command_files/procs_$exchange | grep $shortcode`
	if [ "$is_present" != "" ]; then
		printf "Shortcode present. Lines: \n$is_present\n"
		exit 1;
	fi
	echo "/home/dvctrader/basetrade/scripts/call_run_sim_overnight_perdir.pl $shortcode" >> /media/disk-ephemeral2/command_files/procs_$exchange
elif [ "$action" == "1" ]; then
	cat /media/disk-ephemeral2/command_files/procs_$exchange | grep -v $shortcode >> /media/disk-ephemeral2/command_files/tmp.txt
	mv /media/disk-ephemeral2/command_files/tmp.txt /media/disk-ephemeral2/command_files/procs_$exchange 
else
	cat /media/disk-ephemeral2/command_files/procs_$exchange | awk -F' ' '{print $2}'
fi

if [ "$action" == "0" ] || [ "$action" == "1" ]; then
	echo "Syncing files across 15 and 77 machines"
	scp /media/disk-ephemeral2/command_files/procs_$exchange dvctrader@10.0.1.77:/media/disk-ephemeral2/command_files/
	scp /media/disk-ephemeral2/command_files/procs_$exchange dvctrader@10.0.1.15:/media/disk-ephemeral2/command_files/
fi
