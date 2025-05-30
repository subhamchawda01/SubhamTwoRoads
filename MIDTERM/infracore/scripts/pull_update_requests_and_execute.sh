#!/bin/bash 

. /etc/init.d/functions


YYYYMMDD=`date +"%Y%m%d"` ;

lockfile_access="/spare/local/.update_production_setup_lock.lock" ;
update_requests_file="/spare/local/.update_production_setup_config.cfg" ;
update_requests_temp_file="/home/pengine/.update_production_setup_config.tmp" ;
update_logs="/spare/local/ProdUpdateLogs/production_update_"$YYYYMMDD".log" ;
codebase_base_dir="/home/pengine/codebase/" ; 
main_server="10.23.199.51" ;
main_server_build_script="/home/pengine/utils/build.sh" ; 
database_update_records="/home/pengine/ProdUpdatesDB/all_production_updates_history.txt" ;
database_update_lock="/home/pengine/locks/.update_db.lock" ; 
update_pool="/spare/local/update_pool/" ; 
restoration_pool="/home/pengine/restoration_pool/"
 update_lock_acquired="false" ;
db_lock_acquired="false" ;
host=`hostname` ;
lines="" ;
execution_time="" ;
slack_notification_exec="/home/pengine/utils/send_slack_notification" ;
update_channel="productionupdates" ; 
temp_slack_update_file="/tmp/slack_updates_`date +%s`" ;
gpull="git pull" ;

product_machine_file="/home/pengine/prod/live_configs/machine_shc.txt";
product_machine_lockfile_access="/spare/local/.product_machine_access_lock.lock" ;
prod_mach_lock_acquired="false" ;
product_machine_tmp_file="/home/pengine/prod/live_configs/machine_shc_tmp.txt" ;
temp_update_dir="/tmp/prod_tmp_data" ;


echo "============== INITIATING A NEW INSTNACE FOR THE PRODUCTION UPDATE SCRIPT SERVER @ "`date` "==============" >> $update_logs ; 

ssh_connection=$(ssh -o BatchMode=yes -o ConnectTimeout=8 $main_server echo ok 2>&1);
if [ "$ssh_connection" != "ok" ]; then
	main_server="10.23.74.51";
fi

echo "using database server IP as $main_server " >> $update_logs ; 

trap ctrl_c INT ;

function ctrl_c () {

    echo "Terminating Script On Singal.., $update_lock_acquired, $db_lock_acquired Removing Lock..." >> $update_logs ; 

    if [ "$update_lock_acquired" == "true" ] ; then 
	rm -rf $lockfile_access ; 
    fi 

    if [ "$db_lock_acquired" == "true" ] ; then 
	ssh $main_server "rm -rf $database_update_lock" ; 
    fi 

    exit ;

}

notify_revert () {

    execution_time=`date +"%s"` ; 
	if [ "$1" == "ProductionRevertRequestFailed" ]; then
		echo "Revert failed: no history for update_id $6" > $temp_slack_update_file
	else
    	echo "Reverted on $5 : $2 $7 $6" > $temp_slack_update_file  
	fi
	
    scp $temp_slack_update_file $main_server:/tmp ; 
    ssh $main_server "$slack_notification_exec $update_channel FILE $temp_slack_update_file" >/dev/null 2>/dev/null ; 

    printf "\n===== RevertExecutionDetails ===== \n\nAUTHOR -> $2\nUpdatedFrom -> $3\nRequestTime -> `date -d @$4`\nExecutionTime -> `date -d @$execution_time`\nMachine -> $5\nUpdateDescription -> $7\nUpdatedWithOption -> $8\nRepoUsed -> `echo $9|tr '~' ' '`\nExecName -> ${10}\nErrors -> `grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`\nWarnings -> `grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`\nBuildFailures -> `grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l`\n" >> $update_logs ; 

    echo "Updating Databse Records..." >> $update_logs ; 
    ssh $main_server "echo -e \"\e[31m\"$6 $5 $2 $3 $execution_time $9 $7 $8 ${10}\"\e[0m\" >> $database_update_records" ;

    echo "Verifying Records... " >> $update_logs ;
    if [ `ssh $main_server "grep \"$6\" $database_update_records | wc -l"` -le 0 ] ; then 

#	printf "\n===== RevertExecutionDetails ===== \n\nAUTHOR -> $2\nUpdatedFrom -> $3\nRequestTime -> `date -d @$4`\nExecutionTime -> `date -d @$execution_time`\nMachine -> $5\nUpdateDescription -> $7\nUpdatedWithOption -> $8\nRepoUsed -> `echo $9|tr '~' ' '`\nExecName -> ${10}\nErrors -> `grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`\nWarnings -> `grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`\nBuildFailures -> `grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l`\n" > $temp_slack_update_file ; 

#   echo "RevertFailed ->"$6"^"$2"^"$3"^"`date -d @$4`"^"$5"^"$7"^"$8"^"`echo $9|tr '~' ' '`"^"${10}"^Errors->"`grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`"^Warnings->"`grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`"^BuildFailures->"`grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l` | awk -F"^" '{printf ("|%-25s|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|%-25s|\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)}' > $temp_slack_update_file ;

        scp $temp_slack_update_file $main_server:/tmp ; 
        ssh $main_server "$slack_notification_exec $update_channel FILE $temp_slack_update_file" >/dev/null 2>/dev/null ; 

	echo "Failed To Update Database Records..." >> $update_logs ; 

    fi

    echo "Updated Database Successfully..." >> $update_logs ; 

}

notify_execution () {

    execution_time=`date +"%s"` ;

    #printf "\n===== UpdateExecutionDetails ===== \n\nAUTHOR -> $2\nUpdatedFrom -> $3\nRequestTime -> `date -d @$4`\nExecutionTime -> `date -d @$execution_time`\nMachine -> $5\nUpdateDescription -> $7\nUpdatedWithOption -> $8\nRepoUsed -> `echo $9|tr '~' ' '`\nExecName -> ${10}\nErrors -> `grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`\nWarnings -> `grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`\nBuildFailures -> `grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l`\n" | /bin/mail -s "$1 - $6" -r "pengine@dvcapital.com" "ravi.parikh@tworoads.co.in" ;

    #echo "Execution ->"$6"^"$2"^"$3"^"`date -d @$4`"^"$5"^"$7"^"$8"^"`echo $9|tr '~' ' '`"^"${10}"^Errors->"`grep error $TEMP_SSH_STATUS_CHECK | wc -l`"^Warnings->"`grep warning $TEMP_SSH_STATUS_CHECK | wc -l`"^BuildFailures->"`grep failure $TEMP_SSH_STATUS_CHECK | wc -l` | awk -F"^" '{printf ("|%-25s|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|%-25s|\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)}' > $temp_slack_update_file ;
    echo "Execution on $5 : $2 $7 $6 $bulk_update_id" > $temp_slack_update_file ;
    scp $temp_slack_update_file $main_server:/tmp ;
    ssh $main_server "$slack_notification_exec $update_channel FILE $temp_slack_update_file" >/dev/null 2>/dev/null ;

    printf "\n===== UpdateExecutionDetails ===== \n\nAUTHOR -> $2\nUpdatedFrom -> $3\nRequestTime -> `date -d @$4`\nExecutionTime -> `date -d @$execution_time`\nMachine -> $5\nUpdateDescription -> $7\nUpdatedWithOption -> $8\nRepoUsed -> `echo $9|tr '~' ' '`\nExecName -> ${10}\nErrors -> `grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`\nWarnings -> `grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`\nBuildFailures -> `grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l`\n" >> $update_logs ;

    echo "Updating Databse Records..." >> $update_logs ;
    ssh $main_server "echo $6 $5 $2 $3 $execution_time $9 $7 $8 ${10} $bulk_update_id >> $database_update_records" ;

    echo "Verifying Records... " >> $update_logs ;
    if [ `ssh $main_server "grep \"$6\" $database_update_records | wc -l"` -le 0 ] ; then

	#printf "\n===== UpdateExecutionDetails ===== \n\nAUTHOR -> $2\nUpdatedFrom -> $3\nRequestTime -> `date -d @$4`\nExecutionTime -> `date -d @$execution_time`\nMachine -> $5\nUpdateDescription -> $7\nUpdatedWithOption -> $8\nRepoUsed -> `echo $9|tr '~' ' '`\nExecName -> ${10}\nErrors -> `grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`\nWarnings -> `grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`\nBuildFailures -> `grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l`\n" | /bin/mail -s "FailedToUpdateDatabaseRecords - $6" -r "pengine@dvcapital.com" "ravi.parikh@tworoads.co.in" ;

#	echo "ExecutionFailed ->"$6"^"$2"^"$3"^"`date -d @$4`"^"$5"^"$7"^"$8"^"`echo $9|tr '~' ' '`"^"${10}"^Errors->"`grep \"error\" $TEMP_SSH_STATUS_CHECK | wc -l`"^Warnings->"`grep \"warning\" $TEMP_SSH_STATUS_CHECK | wc -l`"^BuildFailures->"`grep \"failure\" $TEMP_SSH_STATUS_CHECK | wc -l` | awk -F"^" '{printf ("|%-25s|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|%-25s|\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)}' > $temp_slack_update_file ;
        echo "ExecutionFailed on $5 : $2 $7 $6" > $temp_slack_update_file ;
	scp $temp_slack_update_file $main_server:/tmp ;
	ssh $main_server "$slack_notification_exec $update_channel FILE $temp_slack_update_file" >/dev/null 2>/dev/null ;

	echo "Failed To Update Database Records..." >> $update_logs ;

    fi

    echo "Updated Database Successfully..." >> $update_logs ;

}

try_product_machine_access_lock () {
    
    >$TEMP_SSH_STATUS_CHECK ;
    printf "%-60s" "Acquiring product_machine file Lock..." ;
    
    for attempt in {1..20} ;
    do
        if [ ! -e $product_machine_lockfile_access ] ; then 
            touch $product_machine_lockfile_access ;  break ;  
        else 
            if [ `expr $(date +%s) - $(date +%s -r $product_machine_lockfile_access)` -gt "30" ] ; then
                rm $product_machine_lockfile_access
            fi 
        fi ; 
        sleep 1 ;
	echo $attempt > $TEMP_SSH_STATUS_CHECK
    done ;
    
    attempt=`cat $TEMP_SSH_STATUS_CHECK` ;
    >$TEMP_SSH_STATUS_CHECK ;
    if [ "$attempt" != "20" ] ; then
	prod_mach_lock_acquired="true" ;
        printf "Acquired product_machine file Lock \n" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
    else 
        printf "[error] -> FAILED_TO_ACQUIRE_LOCK" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
    fi
}

update_md5sum_entry_for_fr2_servers () {
	
	this_host=$(hostname | awk -F"." '{print $1}');
	tradeinit_path="/home/pengine/prod/live_execs/tradeinit";
	
	if [[ "$this_host" == "sdv-fr2-srv13" || "$this_host" == "sdv-fr2-srv14" || "$this_host" == "sdv-fr2-srv15" || "$this_host" == "sdv-fr2-srv16" ]]; then
		echo "md5sum update tradeinit on fr" >> $update_logs;
		db_file="/spare/local/files/EUREX/eti_md5sum_strategy_algocode_database.db";
		tmp_db_file="/tmp/eti_md5sum_strategy_algocode_database.db_tmp";
		new_db_file="/tmp/eti_md5sum_strategy_algocode_database.db_new";
		newmd5sum=`md5sum "$tradeinit_path" | awk '{print $1}'`;
		changes_description_="tradeinit_update_"`date "+%Y%m%d"`;
		
		cp "$db_file" "$tmp_db_file" >> $update_logs 2>>$update_logs;
		grep -m 1 "#MD5SUM" "$tmp_db_file" >> "$new_db_file" 2>>$update_logs;
		cat "$tmp_db_file" | awk '{ if( ( substr($1,1,1) != "#" ) && ( $1 != "CONSOLE" ) ) print $2, $3;}' | sort -nk 2 | uniq | awk -v newmd5sum=$newmd5sum -v changes_description_=$changes_description_ '{print newmd5sum,$0,changes_description_;}' >> "$new_db_file" 2>>$update_logs
		grep -m 1 "CONSOLE" "$tmp_db_file" >> "$new_db_file" 2>>$update_logs;
		mv "$new_db_file" "$db_file" >> $update_logs 2>>$update_logs;
        rm -rf "$tmp_db_file" >> $update_logs 2>>$update_logs;
        rm -rf "$new_db_file" >> $update_logs 2>>$update_logs;
	fi
}

update_md5sum_entry_for_bsl_servers () {
    
    this_host=$(hostname | awk -F"." '{print $1}');
    tradeinit_path="/home/pengine/prod/live_execs/tradeinit";
    
    if [[ "$this_host" == "sdv-bsl-srv11" || "$this_host" == "sdv-bsl-srv12" || "$this_host" == "sdv-bsl-srv13"  ]]; then
        echo "md5sum update tradeinit on bsl" >> $update_logs;
        db_file="/spare/local/files/ICE/ice_md5sum_strategy_algocode_database.db";
        tmp_db_file="/tmp/eti_md5sum_strategy_algocode_database.db_tmp";
        new_db_file="/tmp/eti_md5sum_strategy_algocode_database.db_new";
        newmd5sum=`md5sum "$tradeinit_path" | awk '{print $1}'`;
        changes_description_="tradeinit_update_"`date "+%Y%m%d"`;
        
        cp "$db_file" "$tmp_db_file" >> $update_logs 2>>$update_logs;
        grep -m 1 "#MD5SUM" "$tmp_db_file" >> "$new_db_file" 2>>$update_logs;
        cat "$tmp_db_file" | awk '{ if( ( substr($1,1,1) != "#" ) && ( $1 != "CONSOLE" ) ) print $2, $3;}' | sort -nk 2 | uniq | awk -v newmd5sum=$newmd5sum -v changes_description_=$changes_description_ '{print newmd5sum,$0,changes_description_;}' >> "$new_db_file" 2>>$update_logs
        grep -m 1 "CONSOLE" "$tmp_db_file" >> "$new_db_file" 2>>$update_logs;
        mv "$new_db_file" "$db_file" >> $update_logs 2>>$update_logs;
        rm -rf "$tmp_db_file" >> $update_logs 2>>$update_logs;
        rm -rf "$new_db_file" >> $update_logs 2>>$update_logs;
    fi
}

while [ true ] 
do 

    echo "Fetching Requests Initiated @ "`date` >> $update_logs ;
	
#  ravi sdv-ny4-srv12.dvcap.local 1432882192 NY14 127545905 UpdatedFromMaster UpdateExecFromRepo infracore cme_ilink_ors
    if [ -e $lockfile_access ] && [ `expr $(date +%s) - $(date +%s -r $lockfile_access)` -gt "60" ] ; then
            rm $lockfile_access ;
    fi

    while [ -s $update_requests_file ] ; do 
        
        while [ -e $lockfile_access ] ; do
            sleep 1;
            if [ -e $lockfile_access ] && [ `expr $(date +%s) - $(date +%s -r $lockfile_access)` -gt "60" ] ; then
               rm $lockfile_access ;
            fi 
        done

        touch $lockfile_access ;
        update_lock_acquired="true" ;

	lines=`head -1 $update_requests_file | tr ' ' '^'` ;
	repo_to_use=`echo $lines | awk -F"^" '{print $8}'` ;
	execs_file_to_build=`echo $lines | awk -F"^" '{print $9}' | tr '~' ' '` ;
	update_option=`echo $lines | awk -F"^" '{print $7}'` ;
	request_id=`echo $lines | awk -F"^" '{print $5}'`; 
	execution_time=`date +"%s"` ;
    bulk_update_id=`echo $lines | awk -F"^" '{print $10}'`;
    echo "Request read" $lines >> $update_logs ;
    echo "BULK UPDATE ID" $bulk_update_id >> $update_logs ;


	grep -v "`echo $lines | tr '^' ' '`" $update_requests_file > $update_requests_temp_file ; 
	mv $update_requests_temp_file $update_requests_file ; 
	chmod 666 $update_requests_file ; 
	rm -rf $lockfile_access  ;
	update_lock_acquired="false" ;
        dvccode=$codebase_base_dir$repo_to_use"/dvccode" ;
        dvctrade=$codebase_base_dir$repo_to_use"/dvctrade" ;

	case "$update_option" in 
            
        "UpdateMachineForProduct")
        
		>$TEMP_SSH_STATUS_CHECK ;
		echo "Acquiring Prod_machine Update Lock..." >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 
		try_product_machine_access_lock	
		input_product_=`echo $execs_file_to_build | awk -F" " '{print $1}'`
		new_machine_=`echo $execs_file_to_build | awk -F" " '{print $2}'`
		backup_dir=`echo $restoration_pool`;
		if [ ! -d $backup_dir ] ; then
            	    mkdir -p $backup_dir >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
		fi
	    #backup the existing version of file
		cp -p -f $product_machine_file $backup_dir"machine_shc.txt#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
		
        updated_entry_=$new_machine_" "$input_product_ ;
        current_entry_=`grep $input_product_ $product_machine_file` ;
        if [ ! -z "$current_entry_" ]; then
        	sed "s/$current_entry_/$updated_entry_/" $product_machine_file  > $product_machine_tmp_file 2>>$TEMP_SSH_STATUS_CHECK  ;
        	mv $product_machine_tmp_file $product_machine_file  2>>$TEMP_SSH_STATUS_CHECK  ;
		else
			echo "$updated_entry_" >> $product_machine_file
		fi
		echo "Veryfying update.." >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
		if [ `grep "$input_product_" "$product_machine_file" | awk -F" " '{print $1}'` == "$new_machine_" ] ; then
		    printf "Shortcode machine map updated successfully \n" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
            cat $TEMP_SSH_STATUS_CHECK >> $update_logs ;
            notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ;
		else
		    printf "error: updating shortcode machine map failed \n" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
            cat $TEMP_SSH_STATUS_CHECK >> $update_logs ;
            ##incase of failure, copy the backup file 
            cp -p -f $backup_dir"machine_shc.txt#"$request_id $product_machine_file
            notify_execution "ProductionUpdateRequestFailed" `echo $lines | tr '^' ' '` ;
        fi
        #delete the lock
        if [ "$prod_mach_lock_acquired" == "true" ] ; then
            rm -f $product_machine_lockfile_access
            prod_mach_lock_acquired="false";
            printf "Removed product_machine file Lock removed \n" >> $update_logs ;
        fi

		;;
	    "UpdateAll"|"UpdateAllProdExecs"|"UpdateAllConfigFiles"|"UpdateAllScripts"|"UpdateExecsFromRepo"|"UpdateConfigsFromRepo"|"UpdateScriptsFromRepo") 
		
		#temporary folder may be deleted. Incase, its not present, create it
		if [ ! -d $temp_update_dir ] ; then
			mkdir -p $temp_update_dir"/bin" 2>> $update_logs ;
			mkdir -p $temp_update_dir"/scripts" 2>> $update_logs ;
			mkdir -p $temp_update_dir"/configs" 2>> $update_logs ;
		fi
	
		>$TEMP_SSH_STATUS_CHECK ;
		echo "Acquiring Database Requests Update Lock..." >> $update_logs ; 
		ssh -q $main_server "for attempt in {1..120} ; do if [ ! -e $database_update_lock ] ; then touch $database_update_lock ; echo \"120 $request_id $execution_time $host\" > $database_update_lock; break ; fi ; sleep 1 ; done ; if [ \$attempt -lt 120 ] ; then printf \"\$attempt\" ; else echo `cat $database_update_lock` ; fi" >$TEMP_SSH_STATUS_CHECK 2>/dev/null ;

		attempt=`cat $TEMP_SSH_STATUS_CHECK | awk '{print $1}'` ; 

		if [ "$attempt" == "120" ] ; then 

		    echo "[ ERROR ] -> FAILED_TO_ACQUIRE_DATABSE_LOCK_FOR_2_MINUTES" >> $update_logs ; 
		    echo "[ FAILED ] -> Update Request Has Failed As Another $update_option Is Being Performed With RequestId ->" `cat $TEMP_SSH_STATUS_CHECK | awk '{print $2}'` ", Since -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $3}'` ", From -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $4}'` >> $update_logs ;

		    if [ $((execution_time - `cat $TEMP_SSH_STATUS_CHECK | awk '{print $3}'`)) -lt 600 ] ; then 

			echo "[ FAILED ] -> Update Request Has Been Delayed As Another $update_option Is Being Performed With RequestId ->" `cat $TEMP_SSH_STATUS_CHECK | awk '{print $2}'` ", Since -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $3}'` ", From -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $4}'` | /bin/mail -s "ErrorDetectedWithProdUpdateSetup" -r "pengine@dvcapital.com" "ravi.parikh@tworoads.co.in" ;
			echo "[ FAILED ] -> Update Request Has Been Delayed As Another $update_option Is Being Performed With RequestId ->" `cat $TEMP_SSH_STATUS_CHECK | awk '{print $2}'` ", Since -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $3}'` ", From -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $4}'` >> $update_logs ; 

                        scp $temp_slack_update_file $main_server:/tmp ; 
                        ssh $main_server "$slack_notification_exec $update_channel FILE $temp_slack_update_file" >/dev/null 2>/dev/null ; 

		    else   

			echo "[ FAILED ] -> Update Request Has Failed As Another $update_option Is Being Performed With RequestId ->" `cat $TEMP_SSH_STATUS_CHECK | awk '{print $2}'` ", Since -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $3}'` ", From -> "`date -d@\`cat $TEMP_SSH_STATUS_CHECK | awk '{print $4}'\`` | /bin/mail -s "ErrorDetectedWithProdUpdateSetup" -r "pengine@dvcapital.com" "ravi.parikh@tworoads.co.in" ;
			echo "[ FAILED ] -> Update Request Has Failed As Another $update_option Is Being Performed With RequestId ->" `cat $TEMP_SSH_STATUS_CHECK | awk '{print $2}'` ", Since -> "`cat $TEMP_SSH_STATUS_CHECK | awk '{print $3}'` ", From -> "`date -d@\`cat $TEMP_SSH_STATUS_CHECK | awk '{print $4}'\`` >> $update_logs ; 

                        scp $temp_slack_update_file $main_server:/tmp ; 
                        ssh $main_server "$slack_notification_exec $update_channel FILE $temp_slack_update_file" >/dev/null 2>/dev/null ; 

                        ssh pengine@10.23.74.51 "$slack_notification_exec $update_channel DATA $slack_data" >/dev/null 2>/dev/null ; 

		    fi 

		    echo "[INFO] -> Restoring Update Request Back" >> $update_logs ; 

		    while [ -e $lockfile_access ] ; 
                    do
                        if [ -e $lockfile_access ] && [ `expr $(date +%s) - $(date +%s -r $lockfile_access)` -gt "60" ] ; then
                           rm $lockfile_access ;
                        fi
                        sleep 1; #wait for lock to be released. But if lock is old, try and delete it.
                    done

		    touch $lockfile_access ; 
		    update_lock_acquired="true" ; 

		    echo `echo $lines | tr '^' ' '` > $update_requests_temp_file ; 
		    echo $update_requests_file >> $update_requests_temp_file ; 

		    mv $update_requests_temp_file $update_requests_file ; 
		    chmod 666 $update_requests_file ; 

		    rm -rf $lockfile_access ; 
		    update_lock_acquired="false" ;

		    echo "[INFO] -> Request Added Back To Queue" ;
                    continue ;           #couldn't acquire the lock, skip this iteration
                    sleep 10 ;           #since an update is running, lets wait before another attempt to acquire the db lock
		fi 

		echo "Database Lock Acquired With -> $attempt Attempts" >> $update_logs 2>>$update_logs ; 

		db_lock_acquired="true" ;

		if [ "$update_option" == "UpdateAll" ] ; then 
                    >$TEMP_SSH_STATUS_CHECK ;

		    echo "Removing Local Files..." >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK; 
			if [ -d $temp_update_dir ] ; then
				rm -rf $temp_update_dir"/bin/"* $temp_update_dir"/scripts/"* $temp_update_dir"/configs/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
			fi
		   #rm -rf $codebase_base_dir$repo_to_use"_install/bin/"* $codebase_base_dir$repo_to_use"_install/scripts/"* $codebase_base_dir$repo_to_use"_install/configs/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;  

                        #build execs on NY11 repo codebase. Incase of devtrade/devmodel, do git pull inside dvctrade repo as well.   
                    if [ "$repo_to_use" == "infracore" ] ; then
                        ssh -q $main_server "rm -rf $codebase_base_dir$repo_to_use\"_install/bin\"* ; cd $dvccode ; $gpull ; $main_server_build_script $codebase_base_dir$repo_to_use" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
                    else
                        ssh -q $main_server "rm -rf $codebase_base_dir$repo_to_use\"_install/bin\"* ; cd $dvccode ; $gpull ; cd $dvctrade ; $gpull; $main_server_build_script $codebase_base_dir$repo_to_use" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
                    fi

                    #get all updated execs
                    rsync -avz $main_server:$codebase_base_dir$repo_to_use"_install/bin" $temp_update_dir >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 
                    
		    #build all configs: get the files from dvccode/configs of DB
                    rsync -avz $main_server:$dvccode"/configs" $temp_update_dir >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;

                     #in devtrade scripts folder doesn't exists
                    if [ "$repo_to_use" != "devtrade"  ] ; then 
                        rsync -avz $main_server:$codebase_base_dir$repo_to_use"/scripts" $temp_update_dir >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
                    fi
                         
                    
		    chmod -R 755 $temp_update_dir ; 
                         #copy all execs in restoration pool and the prod/live_exec
		    for files in `ls $temp_update_dir"/bin/"` ; do 
			cp -r -p $temp_update_dir"/bin/"$files $restoration_pool"live_execs/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 
		    done 

		    for files in `ls $temp_update_dir"/configs/"` ; do 
			cp -r -p $temp_update_dir"/configs/"$files $restoration_pool"live_configs/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 
		    done 

		    for files in `ls $temp_update_dir"/scripts/"` ; do 
			cp -r -p $temp_update_dir"/scripts/"$files $restoration_pool"live_scripts/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 
		    done 

		    ssh -q $main_server "cat /home/pengine/build_status.txt" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ; 

		    if [ `find "$temp_update_dir" -type f | wc -l` -gt 0 ] ; then 

			cp -R -f -p $temp_update_dir"/bin/"* /home/pengine/prod/live_execs/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;  
			cp -R -f -p $temp_update_dir"/configs/"* /home/pengine/prod/live_configs/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;  
			cp -R -f -p $temp_update_dir"/scripts/"* /home/pengine/prod/live_scripts/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ; 

        	cat $TEMP_SSH_STATUS_CHECK >>$update_logs ;
			#tradeinit specific handling for fr2 servers: update md5sum after this update
			if [ -f "/home/pengine/prod/live_execs/tradeinit" ]; then
				update_md5sum_entry_for_fr2_servers >> $update_logs;
                                update_md5sum_entry_for_bsl_servers >> $update_logs;
			fi
			notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 
			
		    else 
            cat $TEMP_SSH_STATUS_CHECK >>$update_logs ;
			notify_execution "ProductionUpdateRequestFailed" `echo $lines | tr '^' ' '` ; 

		    fi 

		elif [ "$update_option" == "UpdateAllProdExecs" ] ; then 
			>$TEMP_SSH_STATUS_CHECK ;
			if [ -d $temp_update_dir ] ; then
				rm -rf $temp_update_dir"/bin/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
			fi
		    
			#rm -rf $codebase_base_dir$repo_to_use"_install/bin/"* ;                        

		    if [ "$repo_to_use" == "infracore" ] ; then 
                        ssh -q $main_server "rm -rf $codebase_base_dir$repo_to_use\"_install/bin\"* ; cd $dvccode ; $gpull ; $main_server_build_script $codebase_base_dir$repo_to_use" >> $update_logs 2>>$update_logs ;
                    else
                        ssh -q $main_server "rm -rf $codebase_base_dir$repo_to_use\"_install/bin\"* ; cd $dvccode ; $gpull ; cd $dvctrade ; $gpull; $main_server_build_script $codebase_base_dir$repo_to_use" >> $update_logs 2>>$update_logs ;
                    fi
		    rsync -avz $main_server:$codebase_base_dir$repo_to_use"_install/bin" $temp_update_dir >> $update_logs 2>>$update_logs ; 

		    chmod -R 755 $temp_update_dir"/bin/" ; 

		    for files in `ls $temp_update_dir"/bin/"` ; do 
			cp -r -p $temp_update_dir"/bin/"$files $restoration_pool"live_execs/"$files"#"$request_id 2>>$update_logs; 
		    done 

		    cp -R -f -p $temp_update_dir"/bin/"* /home/pengine/prod/live_execs/ >> $update_logs 2>>$update_logs ;  

		    >$TEMP_SSH_STATUS_CHECK ;
		    ssh -q $main_server "cat /home/pengine/build_status.txt" > $TEMP_SSH_STATUS_CHECK 2>>$update_logs ;
            cat $TEMP_SSH_STATUS_CHECK >> $update_logs ;
			
			#tradeinit specific handling for fr2 servers: update md5sum after this update
			if [ -f "/home/pengine/prod/live_execs/tradeinit" ]; then
				update_md5sum_entry_for_fr2_servers >> $update_logs;
                                update_md5sum_entry_for_bsl_servers >> $update_logs;
			fi
		    notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 
		    

		elif [ "$update_option" == "UpdateAllConfigFiles" ] ; then 
                    >$TEMP_SSH_STATUS_CHECK ;
			if [ -d $temp_update_dir ] ; then
				rm -rf $temp_update_dir"/configs/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
			fi
		    #rm -rf $codebase_base_dir$repo_to_use"_install/configs/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 

                    ssh -q $main_server "cd $dvccode; $gpull; " >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
                    rsync -avz $main_server:$dvccode"/configs" $temp_update_dir >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;

		    for files in `ls $temp_update_dir"/configs/"` ; do 
			cp -r -p $temp_update_dir"/configs/"$files $restoration_pool"live_configs/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK; 
		    done 

		    chmod -R 755 $temp_update_dir"/configs/" ; 

		    cp -R -f -p $temp_update_dir"/configs/"* /home/pengine/prod/live_configs/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 
                    cat $TEMP_SSH_STATUS_CHECK >> $update_logs ;

		    notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 
 
		elif [ "$update_option" == "UpdateAllScripts" ] ; then
                    >$TEMP_SSH_STATUS_CHECK ; 
			if [ -d $temp_update_dir ] ; then
				rm -rf $temp_update_dir"/scripts/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
			fi;
		    #rm -rf $codebase_base_dir$repo_to_use"_install/scripts/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 

                    ssh -q $main_server "cd $codebase_base_dir$repo_to_use; $gpull; " >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;

                    rsync -avz $main_server:$codebase_base_dir$repo_to_use"/scripts" $temp_update_dir >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ; 

		    chmod -R 755 $temp_update_dir"/scripts/" ; 

		    for files in `ls $temp_update_dir"/scripts/"` ; do 
			cp -r -p $temp_update_dir"/scripts/"$files $restoration_pool"live_scripts/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK; 
		    done 

		    cp -R -f -p $temp_update_dir"/scripts/"* /home/pengine/prod/live_scripts/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK ;
                    cat $TEMP_SSH_STATUS_CHECK >> $update_logs ;  

		    notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ;
                    

		elif [ "$update_option" == "UpdateExecsFromRepo" ] ; then 
                    >$TEMP_SSH_STATUS_CHECK ; 
					if [ -d $temp_update_dir ] ; then
						rm -rf $temp_update_dir"/bin/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
					fi    
                    
                    if [ "$repo_to_use" == "infracore" ] ; then
                        ssh -q $main_server " cd $dvccode ; $gpull ;" >> $update_logs 2>>$update_logs ;
                    else
                        ssh -q $main_server " cd $dvccode ; $gpull ; cd $dvctrade ; $gpull ; " >> $update_logs 2>>$update_logs ;
                    fi

		    ssh -q $main_server "for files in `echo $execs_file_to_build` ; do rm -rf $codebase_base_dir$repo_to_use\"_install/bin/\$files\" ; done ; $main_server_build_script $codebase_base_dir$repo_to_use $execs_file_to_build" >> $update_logs 2>>$update_logs ;
                    
                    for files in `echo $execs_file_to_build` ; do
                        rsync -avz $main_server:$codebase_base_dir$repo_to_use"_install/bin/"$files $temp_update_dir"/bin/" >> $update_logs 2>>$update_logs ; 
                    done

                    chmod -R 755 $temp_update_dir 2>>$update_logs;
                    
                    >$TEMP_SSH_STATUS_CHECK ;
                    ssh -q $main_server "cat /home/pengine/build_status.txt" > $TEMP_SSH_STATUS_CHECK 2>>$update_logs ;
                    cat $TEMP_SSH_STATUS_CHECK >> $update_logs ;
		    
                    all_files_updated="true" ;

		    for files in `echo $execs_file_to_build` ; do 

			if [ `find $temp_update_dir"/bin/" -name "$files" | wc -l` -le 0 ] ; then 
			    all_files_updated="false" ;
			fi 

		    done 

		    if [ "$all_files_updated" == "true" ] ; then 

			for files in `echo $execs_file_to_build` ; do 
				res=`find $temp_update_dir"/bin" -name "$files"`
			    cp -f -p $res $restoration_pool"live_execs/"$files"#"$request_id >> $update_logs 2>>$update_logs ;
			    cp -f -p $res /home/pengine/prod/live_execs/ >> $update_logs 2>>$update_logs ;
			    
				#tradeinit specific handling for fr2 servers: update md5sum after this update
				if [ "$files" == "tradeinit" ]; then
					update_md5sum_entry_for_fr2_servers >> $update_logs;
                                        update_md5sum_entry_for_bsl_servers >> $update_logs;
				fi

			done 

			notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 

		    else 

			notify_execution "ProductionUpdateRequestFailed" `echo $lines | tr '^' ' '` ;  

		    fi 


		elif [ "$update_option" == "UpdateConfigsFromRepo" ] ; then 
                    >$TEMP_SSH_STATUS_CHECK ;
		    if [ -d $temp_update_dir ] ; then
			rm -rf $temp_update_dir"/configs/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
		    fi
	            #update dvccode repo
                    ssh -q $main_server "cd $dvccode ; $gpull ;" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ; 
                    
                    for files in `echo $execs_file_to_build` ; do
                        rsync -avz $main_server:$codebase_base_dir$repo_to_use"/dvccode/configs/"$files $temp_update_dir"/configs" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK;
                    done
                    
		    chmod -R 755 $temp_update_dir ; 

		    all_files_updated="true" ;
                    
                    for files in `echo $execs_file_to_build` ; do 

			if [ `find $temp_update_dir"/configs/" -name "$files" | wc -l` -le 0 ] ; then 
			    all_files_updated="false" ;
			fi 

		    done 

		    if [ "$all_files_updated" == "true" ] ; then 

			for files in `echo $execs_file_to_build` ; do 
				res=`find $temp_update_dir"/configs" -name "$files"` 2>>$update_logs ;
			    cp -f -p  $res $restoration_pool"live_configs/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
			    cp -f -p  $res /home/pengine/prod/live_configs/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;

			done 
                        cat $TEMP_SSH_STATUS_CHECK >>$update_logs ;
			notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 

		    else 
                        cat $TEMP_SSH_STATUS_CHECK >>$update_logs ;
			notify_execution "ProductionUpdateRequestFailed" `echo $lines | tr '^' ' '` ;  

		    fi 

		else 	#UpdateScriptsFromRepo
                    >$TEMP_SSH_STATUS_CHECK ;
		    if [ -d $temp_update_dir ] ; then
			rm -rf $temp_update_dir"/scripts/"* >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
		    fi
                     
		    ssh -q $main_server "cd $codebase_base_dir$repo_to_use ; $gpull ;" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;

                    for files in `echo $execs_file_to_build` ; do
                        rsync -avz $main_server:$codebase_base_dir$repo_to_use"/scripts/"$files $temp_update_dir"/scripts/" >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ; 
                    done

		    chmod -R 755 $temp_update_dir ; 
                    all_files_updated="true" ;  
		    for files in `echo $execs_file_to_build` ; do 

			if [ `find $temp_update_dir"/scripts/" -name "$files" | wc -l` -le 0 ] ; then 
			    all_files_updated="false" ;
			fi 

		    done 
                    
		    if [ "$all_files_updated" == "true" ] ; then 
                        for files in `echo $execs_file_to_build` ; do 
                            res=`find $temp_update_dir"/scripts/" -name "$files"` 2>>$TEMP_SSH_STATUS_CHECK ;
			    cp -f -p $res $restoration_pool"live_scripts/"$files"#"$request_id >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;
			    cp -f -p $res /home/pengine/prod/live_scripts/ >> $TEMP_SSH_STATUS_CHECK 2>>$TEMP_SSH_STATUS_CHECK  ;

			done 
                        cat $TEMP_SSH_STATUS_CHECK >>$update_logs ;
			notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 

		    else 
                        cat $TEMP_SSH_STATUS_CHECK >>$update_logs ;
			notify_execution "ProductionUpdateRequestFailed" `echo $lines | tr '^' ' '` ;  

		    fi 

		fi

		;; 

	    "UpdateGivenExecs"|"UpdateGivenConfigFiles"|"UpdateGivenScripts")  
                 >$TEMP_SSH_STATUS_CHECK ;
		dest_dir="" ;
		backup_dir="" ;

		if [ "$update_option" == "UpdateGivenExecs" ] ; then 

		    dest_dir="/home/pengine/prod/live_execs/" ; 
                    backup_dir=`echo $restoration_pool"live_execs/"`;
                    if [ ! -d $backup_dir ] ; then
                        mkdir -p $backup_dir >> $update_logs 2>>$update_logs ;
                    fi

		elif [ "$update_option" == "UpdateGivenConfigFiles" ] ; then 

		    if_config="true";
            echo "if_config=true" $if_config  >> $update_logs;
            dest_dir="/home/pengine/prod/live_configs/" ; 
                    backup_dir=`echo $restoration_pool"live_configs/"`;
                    if [ ! -d $backup_dir ] ; then
                        mkdir -p $backup_dir >> $update_logs 2>>$update_logs ;
                    fi

		else 

		    dest_dir="/home/pengine/prod/live_scripts/" ; 
                    backup_dir=`echo $restoration_pool"live_scripts/"`;
                    if [ ! -d $backup_dir ] ; then
                        mkdir -p $backup_dir >> $update_logs 2>>$update_logs ;
                    fi

		fi 

		any_single_file_having_issues="false" ;
        is_ors_cfg="false";

		for files in `find $update_pool -type f -name "*$request_id*"` ; do

		    if [ ! -f $files ] || [ ! -s $files ] || [ ! -r $files ] ; then 

			echo "[ERROR] -> FAILED TO EXECUTE THE UPDATES WITH REQUEST ID : $request_id" >> $update_logs 2>>$update_logs ; 
			any_single_file_having_issues="true" ;
			break ;
  
		    fi 
            gzip -d $files;  
            file_backup=`echo $files | awk -F".gz" '{print $1}'`;                           
            files=`echo $files | awk -F"#" '{print $1}'`;
            len=${#files}; echo "len " $len >> $update_logs;
            len=$(($len-7));  echo "len suffix" $len >> $update_logs;
            suff=${files:len:7}; echo "suffix" "$suff" >> $update_logs;
            if [ "$if_config" == "true" ] && [ "ors.cfg" == "$suff" ]; then
                chmod 770 $file_backup; echo "permissions set">> $update_logs;
                is_ors_cfg="true";
            fi

		done 

		if [ "$any_single_file_having_issues" == "false" ] ; then 

		    for files in `find $update_pool -type f -name "*$request_id"` ; do

			destname=`echo $files | awk -F"/" '{print $NF}'| awk -F"#" '{print $1}'` ;

			cp -p -f $files $backup_dir$destname"#"$request_id >> $update_logs 2>>$update_logs ; 
			cp -p -f $files $dest_dir$destname >> $update_logs 2>>$update_logs ; 
            rm -rf $files;
			#tradeinit specific handling for fr2 servers: update md5sum after this update
			if [ "$dest_dir$destname" == "/home/pengine/prod/live_execs/tradeinit" ]; then
				echo "request to update tradeinit " >> $update_logs;
				update_md5sum_entry_for_fr2_servers >> $update_logs;
                                update_md5sum_entry_for_bsl_servers >> $update_logs;
			fi
			
			if [ "$dest_dir" == "/home/pengine/prod/live_configs/" ] && [ "$is_ors_cfg" == "false" ]; then
			   chmod 755 $dest_dir$destname >> $update_logs 2>>$update_logs ;
			fi
		    done 

		    notify_execution "ProductionUpdateRequestExecuted" `echo $lines | tr '^' ' '` ; 

		else 

		    notify_execution "ProductionUpdateRequestFailed" `echo $lines | tr '^' ' '` ; 
                fi
                ;;
            
	    "RevertTo"|"RevertLast"|"BulkRevert") 
    

        #so incase it is RevertLast we would change the request_id to the previous one and proceed.

		echo "Initiating Revert $update_option With Request Id -> $request_id" >> $update_logs 2>>$update_logs ; 

		`find $restoration_pool"live_execs/" -name "*#$request_id"` ; 
		`find $restoration_pool"live_configs/" -name "*#$request_id"` ; 

		if [ `find $restoration_pool"live_execs/" -name "*#$request_id" | wc -l` -gt 0 ] || [ `find $restoration_pool"live_configs/" -name "*#$request_id" | wc -l` -gt 0 ] || [ `find $restoration_pool"live_scripts/" -name "*#$request_id" | wc -l` -gt 0 ] ; then 

		    for files in `find $restoration_pool"live_execs/" -name "*#$request_id"` ; do 
                        update_id=$request_id;
			echo "Reverting Execs -> $files ..." ; 
			destname=`echo $files | awk -F"/" '{print $NF}' | awk -F"#" '{print $1}'` ; 
             if [ "$update_option" == "RevertLast" ] || [ "$update_option" == "BulkRevert" ] ; then             
               update_id=$(for en in `ls -lrt /home/pengine/restoration_pool/live_execs/ | grep $destname | awk '{print $9}'` ; do  if [ "$en" == "$destname"#"$request_id" ] ; then echo "$prev"; fi; prev="$en"; done | awk -F"#" '{print $2}')
               
               echo "$update_option input req id-> $request_id, previous req id-> $var" >> $update_logs 2>>$update_logs ; 
             fi
              file_to_revert="$restoration_pool""live_execs/""$destname""#""$update_id";
			cp -p -f $file_to_revert /home/pengine/prod/live_execs/$destname >> $update_logs 2>>$update_logs ; 
			
			#tradeinit specific handling for fr2 servers: update md5sum after this update
			if [ "$destname" == "tradeinit" ]; then
				update_md5sum_entry_for_fr2_servers >> $update_logs;
                                update_md5sum_entry_for_bsl_servers >> $update_logs;
			fi
		    done 

		    for files in `find $restoration_pool"live_configs/" -name "*#$request_id"` ; do
                        update_id=$request_id;
			echo "Reverting Configs -> $files ..." ; 
			destname=`echo $files | awk -F"/" '{print $NF}' | awk -F"#" '{print $1}'` ; 
            if [ "$update_option" == "RevertLast" ] || [ "$update_option" == "BulkRevert" ] ; then             
               update_id=$(for en in `ls -lrt /home/pengine/restoration_pool/live_configs/ | grep $destname | awk '{print $9}'` ; do  if [ "$en" == "$destname"#"$request_id" ] ; then echo "$prev"; fi; prev="$en"; done | awk -F"#" '{print $2}')

               echo "$update_option input req id-> $request_id, previous req id-> $var" >> $update_logs 2>>$update_logs ; 
             fi
              file_to_revert="$restoration_pool""live_configs/""$destname""#""$update_id";
			cp -p -f $file_to_revert /home/pengine/prod/live_configs/$destname >> $update_logs 2>>$update_logs ; 

		    done 

		    for files in `find $restoration_pool"live_scripts/" -name "*#$request_id"` ; do 
                        update_id=$request_id;
			echo "Reverting Scripts -> $files ..." ; 
			destname=`echo $files | awk -F"/" '{print $NF}' | awk -F"#" '{print $1}'` ; 
             if [ "$update_option" == "RevertLast" ] || [ "$update_option" == "BulkRevert" ] ; then             
               update_id=$(for en in `ls -lrt /home/pengine/restoration_pool/live_scripts/ | grep $destname | awk '{print $9}'` ; do  if [ "$en" == "$destname"#"$request_id" ] ; then echo "$prev"; fi; prev="$en"; done | awk -F"#" '{print $2}')

               echo "$update_option input req id-> $request_id, previous req id-> $var" >> $update_logs 2>>$update_logs ; 
             fi
              file_to_revert="$restoration_pool""live_scripts/""$destname""#""$update_id";
			cp -p -f $file_to_revert /home/pengine/prod/live_scripts/$destname >> $update_logs 2>>$update_logs ; 

		    done 

		    notify_revert "ProductionRevertRequestExecuted" `echo $lines | tr '^' ' '` ; 

		else 

		    notify_revert "ProductionRevertRequestFailed" `echo $lines | tr '^' ' '` ; 

		fi 

		;; 

	    *) 

		;; 

	esac ; 

        if [ "$prod_mach_lock_acquired" == "true" ] ; then
          rm -f $product_machine_lockfile_access
          prod_mach_lock_acquired="false";
        fi

        if [ "$db_lock_acquired" == "true" ] ; then 	
	    ssh $main_server "rm -rf $database_update_lock" ;
	    db_lock_acquired="false" ;
        fi
        if [ "$update_lock_acquired" == "true" ] ; then 
	    rm -rf $lockfile_access ; 
	    update_lock_acquired="false" ;
	fi 
    done 

    sleep 45 ; 

done 
