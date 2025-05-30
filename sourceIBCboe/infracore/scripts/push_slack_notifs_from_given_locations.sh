#!/bin/bash

export http_proxy=127.0.0.1:8181
export https_proxy=127.0.0.1:8181

mkdir -p /tmp/push_slack_tempfiles/ ;
slack_exec=/home/pengine/prod/live_execs/send_slack_notification ;
tranche_id_2_strategy_map=/home/dvctrader/LiveConfigs/trancheid_2_strategyname_map

##restart handling in middle of day
today=`date +"%Y%m%d"` ;
rv_log_file=/spare/local/logs/alllogs/MediumTerm/rv_execlogic_dbglog.$today
nseet_log_file=/spare/local/logs/alllogs/MediumTerm/earnings_execlogic_dbglog.$today
nsewst_log_file=/spare/local/logs/alllogs/MediumTerm/weeklyshortgamma60_execlogic_dbglog.$today
nseslb_log_file=/spare/local/logs/alllogs/MediumTerm/slb_execlogic_dbglog.$today
nsedisptr_log_file=/spare/local/logs/alllogs/MediumTerm/dispersion_execlogic_dbglog.$today
temp_slack_file=/tmp/push_slack_tempfiles/slack_$today ;
temp_rv_slack_file=/tmp/push_slack_tempfiles/rv_slack_$today ;
temp_earning_slack_file=/tmp/push_slack_tempfiles/nseet_slack_$today ;
temp_wst_slack_file=/tmp/push_slack_tempfiles/nsewst_slack_$today;
temp_slb_slack_file=/tmp/push_slack_tempfiles/nseslb_slack_$today;
temp_disptr_slack_file=/tmp/push_slack_tempfiles/nsedisptr_slack_$today;
cat /spare/local/MDSlogs/MT_SPRD_DBGLOGS/LOG_$today | strings | grep "SLACK"  > $temp_slack_file
cat "$rv_log_file" | strings | grep "SLACK" > $temp_rv_slack_file ;
cat "$nseet_log_file" | strings | grep "SLACK" > $temp_earning_slack_file;
cat "$nsewst_log_file" | strings | grep "SLACK" > $temp_wst_slack_file;
cat "$nseslb_log_file" | strings | grep "SLACK" > $temp_slb_slack_file;
cat "$nsedisptr_log_file" | strings | grep "SLACK" > $temp_disptr_slack_file;

while [ true ] ; do 
	
	today=`date +"%Y%m%d"` ;
	temp_slack_file=/tmp/push_slack_tempfiles/slack_$today ;
	#remove older files
	rm `ls /tmp/push_slack_tempfiles/* | grep -v "$today" 2>/dev/null` 2>/dev/null
	
	cat /spare/local/MDSlogs/MT_SPRD_DBGLOGS/LOG_$today | strings | grep "SLACK" > /tmp/.push_slack.txt ;
	diff -u $temp_slack_file /tmp/.push_slack.txt  ;

	for lines in `diff -u $temp_slack_file /tmp/.push_slack.txt | grep -v "@@"  | grep -v "/tmp" | grep "^+" | tr ':' ' ' | tr ' ' '~'` ; do
    	datastring=`echo "$lines" | tr '~' '\t' | awk -F"SLACK" '{print "PairTrading \t"$2}'` ;
    	$slack_exec "nsemed" DATA "$datastring" ;
	done
	cat /tmp/.push_slack.txt > $temp_slack_file
	
	##RV slack setup
	
	temp_rv_slack_file=/tmp/push_slack_tempfiles/rv_slack_$today ;
	rv_log_file=/spare/local/logs/alllogs/MediumTerm/rv_execlogic_dbglog.$today
	
	cat "$rv_log_file" | strings | grep "SLACK" > /tmp/push_slack_tempfiles/.rv_push_slack.txt ;
	diff -u $temp_rv_slack_file  /tmp/push_slack_tempfiles/.rv_push_slack.txt ;
	
	for lines in `diff -u $temp_rv_slack_file /tmp/push_slack_tempfiles/.rv_push_slack.txt | grep -v "@@"  | grep -v "/tmp"| grep -v "^++" | grep "^+"| tr -d ' '  | tr '\r\t' '~'` ; do
	    datastring=`echo "$lines" | tr '~' '\t' | awk -F"SLACK" '{print "RVTrading \t"$2}'` ; 
            strat_id=$(echo -e $datastring | awk 'BEGIN{FS=" "}{shc=$2;split(shc,sid,"_"); print sid[2]}')
            stratname_channel=( $(cat $tranche_id_2_strategy_map | awk -v sid="$strat_id" 'BEGIN{FS="\t"; OFS=" "}{if($1==sid){print($2,$3)}}') )
            datastring=${stratname_channel[0]}"\t"$datastring
            $slack_exec ${stratname_channel[1]} DATA "$datastring" ;
            if [[ "${stratname_channel[1]}" == "" ]]; then  $slack_exec "nsemed" DATA "$datastring" ;fi

	    $slack_exec "banknifty-rv" DATA "$datastring" ;
	done
	cat /tmp/push_slack_tempfiles/.rv_push_slack.txt >  $temp_rv_slack_file ;
	
	##Earning slack setup
	temp_earning_slack_file=/tmp/push_slack_tempfiles/nseet_slack_$today ;
	nseet_log_file=/spare/local/logs/alllogs/MediumTerm/earnings_execlogic_dbglog.$today;
	
	cat "$nseet_log_file" | strings | grep "SLACK" > /tmp/push_slack_tempfiles/.nseet_slack_.txt
	diff -u $temp_earning_slack_file /tmp/push_slack_tempfiles/.nseet_slack_.txt
	
	for lines in `diff -u $temp_earning_slack_file /tmp/push_slack_tempfiles/.nseet_slack_.txt | grep -v "@@"  | grep -v "/tmp"| grep -v "^++" | grep "^+"| tr -d ' '  | tr '\r\t' '~'` ; do
		datastring=`echo "$lines" | tr '~' '\t' | awk -F"SLACK" '{print "EarningTrading \t"$2}'` ; 
	    $slack_exec "midterm-earnings" DATA "$datastring" ;
	done
	cat /tmp/push_slack_tempfiles/.nseet_slack_.txt > $temp_earning_slack_file
	
	##WeeklyShortGama slack setup
	temp_wst_slack_file=/tmp/push_slack_tempfiles/nsewst_slack_$today;
	nsewst_log_file=/spare/local/logs/alllogs/MediumTerm/weeklyshortgamma60_execlogic_dbglog.$today
	
	cat "$nsewst_log_file" | strings | grep "SLACK" > /tmp/push_slack_tempfiles/.nsewst_slack_.txt
	diff -u $temp_wst_slack_file /tmp/push_slack_tempfiles/.nsewst_slack_.txt
	
	for lines in `diff -u $temp_wst_slack_file /tmp/push_slack_tempfiles/.nsewst_slack_.txt | grep -v "@@"  | grep -v "/tmp"| grep -v "^++" | grep "^+"| tr -d ' '  | tr '\r\t' '~'` ; do
		datastring=`echo "$lines" | tr '~' '\t' | awk -F"SLACK" '{print $2}'` ; 
                strat_id=$(echo -e $datastring | awk 'BEGIN{FS=" "}{shc=$2;split(shc,sid,"_"); print sid[2]}')
                stratname_channel=( $(cat $tranche_id_2_strategy_map | awk -v sid="$strat_id" 'BEGIN{FS="\t"; OFS=" "}{if($1==sid){print($2,$3)}}') )
                datastring=${stratname_channel[0]}"\t"$datastring
                $slack_exec ${stratname_channel[1]} DATA "$datastring" ;
	    #$slack_exec "weeklysg" DATA "$datastring" ;
            if [[ "${stratname_channel[1]}" == "" ]]; then  $slack_exec "nsemed" DATA "$datastring" ;fi
	done
	cat /tmp/push_slack_tempfiles/.nsewst_slack_.txt > $temp_wst_slack_file
	
	##SLB slack setup
	temp_slb_slack_file=/tmp/push_slack_tempfiles/nseslb_slack_$today;
	nseslb_log_file=/spare/local/logs/alllogs/MediumTerm/slb_execlogic_dbglog.$today
	
	cat "$nseslb_log_file" | strings | grep "SLACK" > /tmp/push_slack_tempfiles/.nseslb_slack_.txt
	diff -u $temp_slb_slack_file /tmp/push_slack_tempfiles/.nseslb_slack_.txt
	
	for lines in `diff -u $temp_slb_slack_file /tmp/push_slack_tempfiles/.nseslb_slack_.txt | grep -v "@@"  | grep -v "/tmp"| grep -v "^++" | grep "^+"| tr -d ' '  | tr '\r\t' '~'` ; do
		datastring=`echo "$lines" | tr '~' '\t' | awk -F"SLACK" '{print "SLBTrading \t"$2}'` ; 
	    $slack_exec "nsemed" DATA "$datastring" ;
	done
	cat /tmp/push_slack_tempfiles/.nseslb_slack_.txt > $temp_slb_slack_file
	
        ##DISPERSION slack setup
        temp_disptr_slack_file=/tmp/push_slack_tempfiles/nsedisptr_slack_$today;
        nsedisptr_log_file=/spare/local/logs/alllogs/MediumTerm/dispersion_execlogic_dbglog.$today

        cat "$nsedisptr_log_file" | strings | grep "SLACK" > /tmp/push_slack_tempfiles/.nsedisptr_slack_.txt
        diff -u $temp_disptr_slack_file /tmp/push_slack_tempfiles/.nsedisptr_slack_.txt

        for lines in `diff -u $temp_disptr_slack_file /tmp/push_slack_tempfiles/.nsedisptr_slack_.txt | grep -v "@@"  | grep -v "/tmp"| grep -v "^++" | grep "^+"| tr -d ' '  | tr '\r\t' '~'` ; do
                datastring=`echo "$lines" | tr '~' '\t' | awk -F"SLACK" '{print "DispersionTrading \t"$2}'` ;
            $slack_exec "midterm-dispersion" DATA "$datastring" ;
        done
        cat /tmp/push_slack_tempfiles/.nsedisptr_slack_.txt > $temp_disptr_slack_file


	sleep 10 ; 

done 
