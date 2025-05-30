#!/bin/bash 

. /etc/init.d/functions

declare -A options_map ;
declare -A servers_map ;
declare -a servers_list ; 
declare -a input_given_file_exec_from_repo ; 
declare -a input_specific_exec_or_config_file ; 
declare -a input_products_or_machines ;
declare -A reverse_ip_map ; 

update_option="q" ;
update_now_option="N" ; 
repo_to_update="NONE" ;
update_id=`date +%N` ;
updated_from=`hostname | awk -F"." '{print $1}'` ;
current_tty=`echo $SSH_TTY | sed 's/\/dev\///g'`; 
author="INVALID" ;
authorized_author_list="ravi rakesh gaurav vedant anshul hrishav diwakar ankit kaushik abhishek pranjal rishabh puru bot guest uttkarsh nishank rahul praful kartik keshav ajindal rahul_n mansi abdul animesh_g surjeet" ;
request_time=`date +"%s"` ; 

input_given_file_exec_from_repo="NONE" ;
input_specific_exec_or_config_file="NONE" ; 
remote_lockfile_access="/spare/local/.update_production_setup_lock.lock" ; 
remote_update_file="/spare/local/.update_production_setup_config.cfg" ; 
update_description="NONE" ;
machine="" ;
remote_user="dvcinfra@" ;
TEMP_SSH_STATUS_CHECK="/tmp/.update_production_setup_ssh_status_"$update_id".txt" ; 
REMOTE_POOL_FOR_UPDATES="/spare/local/update_pool/" ;
lock_acquired="false" ;
db_lock_acquired="false" ;
database_server="10.23.5.67" ;
database_update_records="/home/pengine/ProdUpdatesDB/all_production_updates_history.txt" ;
database_request_records="/spare/local/ProdUpdatesDB/all_production_updates_requests.txt" ;
database_request_update_lock="/spare/local/.update_request_db.lock" ;
slack_notification_exec="/home/pengine/utils/send_slack_notification" ;
update_channel="productionupdates" ;
temp_slack_update_requests_file="/tmp/slack_update_requests_$update_id" ; 
product_machine_file="/home/pengine/prod/live_configs/machine_shc.txt";
product_machine_lockfile_access="/spare/local/.product_machine_access_lock.lock" ;
prod_mach_lock_acquired="false" ;


trap ctrl_c INT ; 

cleanup () {

    if [ "$db_lock_acquired" == "true" ] ; then 
	ssh $remote_user$database_server "rm -rf $database_request_update_lock" >/dev/null 2>/dev/null & 
	db_lock_acquired="false" ;
    fi 

    if [ "$lock_acquired" == "true" ] ; then 

	>$TEMP_SSH_STATUS_CHECK ; 
	printf "\n%-60s" "Releasing Lock...Cleaning Up..." ; 

	ssh -q $remote_user$1 "rm -f $remote_lockfile_access | echo true" > $TEMP_SSH_STATUS_CHECK & 
	spinner $! ; 
	action "" [ -s $TEMP_SSH_STATUS_CHECK ] ; 

	lock_acquired="false" ;

    fi 

    rm -rf $TEMP_SSH_STATUS_CHECK ; 

}

cleanup_and_exit () {

    if [ "$db_lock_acquired" == "true" ] ; then 
	ssh $remote_user$database_server "rm -rf $database_request_update_lock" >/dev/null 2>/dev/null & 
	db_lock_acquired="false" ;
    fi 

  
    if [ "$lock_acquired" == "true" ] ; 
    then

	>$TEMP_SSH_STATUS_CHECK ; 
	printf "\n%-60s" "Releasing Lock...Cleaning Up..." ; 

	ssh -q $remote_user$1 "rm -f $remote_lockfile_access | echo true" > $TEMP_SSH_STATUS_CHECK & 
	spinner $! ; 
	action "" [ -s $TEMP_SSH_STATUS_CHECK ] ; 

        lock_acquired="false" ;

    fi 

    rm -rf $TEMP_SSH_STATUS_CHECK ; 
    echo "THANK_YOU_FOR_USING_SCRIPT !!" ;

    exit ; 

}

function ctrl_c () {

    if [ "$db_lock_acquired" == "true" ] ; then 
	ssh $remote_user$database_server "rm -rf $database_request_update_lock" >/dev/null 2>/dev/null & 
	db_lock_acquired="false" ;
    fi 

    if [ "$lock_acquired" == "true" ] ; then 
	ssh $remote_user$machine "rm -rf $remote_lockfile_access" >/dev/null 2>/dev/null & 
	lock_acquired="false";
    fi   

    printf "\n\nScript Execution Abrupted !!\n" ; 
    cleanup_and_exit $machine ; 

}

spinner()
{
    local pid=$1 ;
    local delay=0.30 ;
    local spinstr='|/-\\' ; 

    while [ "$(ps a | awk '{print $1}' | grep -w $pid)" ]; do
        local temp=${spinstr#?}
        printf "  [%c]  " "$spinstr"
        local spinstr=$temp${spinstr%"$temp"}
        sleep $delay
        printf "\b\b\b\b\b\b\b"
    done
    printf "    \b\b\b\b"
}

update_failure () {
	echo $1;
	if [ `echo $servers_update_failed_for | grep $2 | wc -l` -le 0 ]; then
		servers_update_failed_for=$servers_update_failed_for" "$2
	fi
}

script_failure () {

    echo $1 ;

    if [ "$db_lock_acquired" == "true" ] ; then 
	ssh $remote_user$database_server "rm -rf $database_request_update_lock" >/dev/null 2>/dev/null & 
	db_lock_acquired="false" ;
    fi 

    [ "$lock_acquired" != "true" ] || cleanup_and_exit $machine ; 

    exit ; 
}

initialize () {
	
	#disable unused options for now, may be needed in future - Abhishek
    options_map[h]="HelpOnOptions" ;
    #options_map[0]="UpdateAll" ;
    #options_map[1]="UpdateAllProdExecs" ;
    #options_map[2]="UpdateAllConfigFiles" ;
    #options_map[3]="UpdateAllScripts" ;
    #options_map[4]="UpdateExecsFromRepo" ; 
    #options_map[5]="UpdateConfigsFromRepo" ; 
    #options_map[6]="UpdateScriptsFromRepo" ; 
    options_map[7]="UpdateGivenExecs" ;
    options_map[8]="UpdateGivenConfigFiles" ;
    options_map[9]="UpdateGivenScripts" ;
    #options_map[M]="GetMachineForProducts" ; 
    #options_map[S]="GetProductsForMachine" ;
    options_map[B]="BrowseUpdatesHistory" ; 
    options_map[R]="RevertTo" ; 
    #options_map[P]="ProductManagement" ;
    options_map[q]="Quit" ;
    ###############################################################################################################################
    options_map[r]="RevertLast";
    options_map[b]="BulkRevert";
    ###############################################################################################################################

  #TODO - There should only be 1 file in entire codebase from which server ips should be accessed 
    servers_map[NY11]="10.23.5.67" ;
    servers_map[NY12]="10.23.74.52" ;
    servers_map[NY13]="10.23.74.53" ;
    servers_map[NY14]="10.23.74.54" ;
    servers_map[NY15]="10.23.74.55" ;
    servers_map[CRT11]="10.23.142.51" ;
    servers_map[CFE11]="10.23.74.61" ;
    servers_map[CFE12]="10.23.74.62" ;
    #servers_map[CFE13]="10.23.74.63" ;
    servers_map[FR13]="10.23.102.53" ;
    servers_map[FR14]="10.23.102.54" ;
    servers_map[FR15]="10.23.102.55" ;
    servers_map[FR16]="10.23.102.56" ;
    servers_map[CHI13]="10.23.82.53" ;
    servers_map[CHI14]="10.23.82.54" ;
    servers_map[CHI15]="10.23.82.55" ;
    servers_map[CHI16]="10.23.82.56" ;
    servers_map[BSL11]="10.23.52.51" ;
    servers_map[BSL12]="10.23.52.52" ;
    servers_map[BSL13]="10.23.52.53" ;
    servers_map[TOR11]="10.23.182.51" ;
    servers_map[TOR12]="10.23.182.52" ;
    servers_map[TOR13]="10.23.182.53" ;
    servers_map[BMF11]="10.230.63.11" ;
    servers_map[BMF12]="10.230.63.12";
    servers_map[BMF13]="10.230.63.13" ;
    servers_map[BMF14]="10.230.63.14" ;
    servers_map[BMF15]="10.230.63.15" ;
    servers_map[MOS11]="172.26.33.227" ;
    servers_map[MOS12]="172.26.33.232";
    servers_map[MOS13]="172.26.33.226";
    servers_map[HK11]="10.152.224.145" ;
	servers_map[HK12]="10.152.224.146" ;
	servers_map[HKALL]="10.152.224.145 10.152.224.146" ;
    servers_map[OSE13]="10.134.73.213" ;
    servers_map[OSE14]="10.134.73.214" ;
    servers_map[ASX11]="10.23.43.51" ; 
    servers_map[ASX12]="10.23.43.52" ; 
    servers_map[IND11]="10.23.115.61";
    servers_map[IND12]="10.23.115.62";
    servers_map[IND13]="10.23.115.63";
    servers_map[IND14]="10.23.115.64" ;
    servers_map[IND15]="10.23.115.65" ;
    servers_map[IND16]="10.23.115.81" ;
    servers_map[IND17]="10.23.115.82" ;
    servers_map[IND18]="10.23.115.83" ;
    servers_map[IND19]="10.23.115.69" ;
    servers_map[INDALL]="10.23.115.61 10.23.115.62 10.23.115.63 10.23.115.64 10.23.115.65 10.23.115.81 10.23.115.82 10.23.115.83 10.23.115.69";

    servers_map[SGX11]="10.23.26.51" ;
    servers_map[SGX12]="10.23.26.52" ;
    servers_map[SGXALL]="10.23.26.51 10.23.26.52";
    servers_map[BMFALL]="10.230.63.11 10.230.63.12 10.230.63.13 10.230.63.14 10.230.63.15" ;
    servers_map[CHIALL]="10.23.82.53 10.23.82.54 10.23.82.55 10.23.82.56"
    servers_map[FRALL]="10.23.102.53 10.23.102.54 10.23.102.55 10.23.102.56" ;
    servers_map[BSLALL]="10.23.52.51 10.23.52.52 10.23.52.53" ;
    servers_map[TORALL]="10.23.182.51 10.23.182.52 10.23.182.53" ; 
    servers_map[OSEALL]="10.134.73.213 10.134.73.214" ;

    servers_map[MOSALL]="172.26.33.227 172.26.33.232 172.26.33.226" ;
    servers_map[DEVALL]="10.23.5.67 10.23.74.52 10.23.74.53 10.23.74.54 10.23.74.55" ;
    servers_map[ALLSERVERS]="10.23.5.67 10.23.74.52 10.23.74.53 10.23.74.54 10.23.74.55" ;

    reverse_ip_map[10.23.5.67]="NY11" ;
    reverse_ip_map[10.23.74.52]="NY12" ;
    reverse_ip_map[10.23.74.53]="NY13" ;
    reverse_ip_map[10.23.74.54]="NY14" ;
    reverse_ip_map[10.23.74.55]="NY15" ;
    reverse_ip_map[10.23.142.51]="CRT11" ;
    reverse_ip_map[10.23.74.61]="CFE11" ;
    reverse_ip_map[10.23.74.62]="CFE12" ;
    #reverse_ip_map[10.23.74.63]="CFE13" ;
    reverse_ip_map[10.23.102.53]="FR13" ;
    reverse_ip_map[10.23.102.54]="FR14" ;
    reverse_ip_map[10.23.102.55]="FR15" ;
    reverse_ip_map[10.23.102.56]="FR16" ;
    reverse_ip_map[10.23.82.53]="CHI13" ;
    reverse_ip_map[10.23.82.54]="CHI14" ;
    reverse_ip_map[10.23.82.55]="CHI15" ;
    reverse_ip_map[10.23.82.56]="CHI16" ;
    reverse_ip_map[10.23.52.51]="BSL11" ;
    reverse_ip_map[10.23.52.52]="BSL12" ;
    reverse_ip_map[10.23.52.53]="BSL13" ;
    reverse_ip_map[10.23.182.51]="TOR11" ;
    reverse_ip_map[10.23.182.52]="TOR12" ;
    reverse_ip_map[10.23.182.53]="TOR13" ;
    reverse_ip_map[10.230.63.11]="BMF11" ;
    reverse_ip_map[10.230.63.12]="BMF12" ;
    reverse_ip_map[10.230.63.13]="BMF13" ;
    reverse_ip_map[10.230.63.14]="BMF14" ;
    reverse_ip_map[10.230.63.15]="BMF15" ;
    reverse_ip_map[172.26.33.227]="MOS11" ;
    reverse_ip_map[172.26.33.232]="MOS12" ;
    reverse_ip_map[172.26.33.226]="MOS13";
    reverse_ip_map[10.152.224.145]="HK11" ;
    reverse_ip_map[10.152.224.146]="HK12" ;
    reverse_ip_map[10.134.73.213]="OSE13" ;
    reverse_ip_map[10.134.73.214]="OSE14" ;
    reverse_ip_map[10.23.43.51]="ASX11" ;
    reverse_ip_map[10.23.43.52]="ASX12" ;
    reverse_ip_map[10.23.115.61]="IND11" ;
    reverse_ip_map[10.23.115.62]="IND12" ;
    reverse_ip_map[10.23.115.63]="IND13" ;
    reverse_ip_map[10.23.115.64]="IND14" ;
    reverse_ip_map[10.23.115.65]="IND15" ;
    reverse_ip_map[10.23.115.81]="IND16" ;
    reverse_ip_map[10.23.115.82]="IND17" ;
    reverse_ip_map[10.23.115.83]="IND18" ;
    reverse_ip_map[10.23.115.69]="IND19" ;
    reverse_ip_map[10.23.26.51]="SGX11" ;
    reverse_ip_map[10.23.26.52]="SGX12" ;
}

fetch_repo () {

    printf "$1 q to exit -> _________\b\b\b\b\b\b\b\b\b" ; 

    while [ true ] ; do 
	read -e repo_to_update ; 
	[ "$repo_to_update" != "infracore" -a "$repo_to_update" != "devtrade" -a "$repo_to_update" != "devmodel" -a "$repo_to_update" != "q" ] || break ;
	printf "\nInvalid Repo Or Option Try Again..., $1 q to exit -> _________\b\b\b\b\b\b\b\b\b" ;
    done 

    [ "$repo_to_update" != "q" ] || script_failure "THANK_YOU_FOR_USING_SCRIPT !!" ; 

}

fetch_branch () {

    printf "$1 q to exit -> _________\b\b\b\b\b\b\b\b\b" ; 

    while [ true ] ; do 
	read -e repo_to_update ; 

        if [ "$repo_to_update" == "q" ] || [ `echo $repo_to_update | awk '{print NF}'` -eq 1 ] 
        then 

            break ;

        fi 

	printf "\nInvalid Branch..Try Again... -> _________\b\b\b\b\b\b\b\b\b" ;
    done 

    [ "$repo_to_update" != "q" ] || script_failure "THANK_YOU_FOR_USING_SCRIPT !!" ; 

}

confirm_revert_type () {
    [ ! -z $servers_update_failed_for ] &&  printf "\n Revert request failed for $servers_update_failed_for . Please retry. \n ";
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
	printf "Acquired product_machine file Lock \n";
    else 
	script_failure "[ERROR] -> FAILED_TO_ACQUIRE_LOCK" ;
    fi
}


display_machine_for_product () {
    input_product=$1;
    printf "Current machine for $input_product : ";
    machine=`grep -w $input_product $product_machine_file | awk '{print $1}'`;
    if [ ! -z $machine ] ; then
	printf "$machine \n";
    else
	printf "No machine found. Please check product details \n" 
    fi
}

get_all_products_for_machine () {
    input_machine=$1
    
    printf "Current products on $input_machine : ";
    all_products=`grep $input_machine $product_machine_file | tr ' ' '#'`;
    all_products=($all_products)
    if [ ! -z $all_products ] ; then
        for line in `echo ${all_products[*]}`
        do
      	    echo $line | awk -F"#" '{printf $2" "}'
        done
        printf "\n"
    else
        printf "No machine found. Please check machine details \n" 
    fi
}

revert_updates () {

    update_id=$1 ; 

    >$TEMP_SSH_STATUS_CHECK ;
    ssh $remote_user$database_server "cat $database_update_records | grep \"$revert_to_request_id\"" > $TEMP_SSH_STATUS_CHECK ; 

    [ -s $TEMP_SSH_STATUS_CHECK ] || script_failure "[ERROR] -> THERE ARE NO UPDATES AVAILABLE WITH GIVEN REQUEST ID $1 TO REVERT TO" ; 
	
	
	
    fetch_author_name "Please Enter Your First Name ( all in small letters ) -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 

    printf "Please Provide One Word Update Description -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
    read update_description ; 

    while [ `echo $update_description | awk '{print NF}'` -ne 1 ] 
    do
	
	printf "You Entered `echo $update_description | awk '{print NF}'` Words, Just One Plaese.. ( use _ to club words ) -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
	read update_description ; 

    done 
	
	#get all revert lines matching the revert_id provided
    for reverts in `cat $TEMP_SSH_STATUS_CHECK | grep -v "RevertTo" | grep -v "RevertLast" | grep -v "BulkRevert" | tr ' ' '^'` ; do 
    update_id=`echo $reverts | awk -F"^" '{print $1}'`;
	server=`echo $reverts | awk -F"^" '{print $2}'` ;
	machine=`echo ${servers_map[$server]}` ; 
	repo_to_update=`echo $reverts | awk -F"^" '{print $6}'` ;
	input_given_file_exec_from_repo=`echo $reverts | awk -F"^" '{print $9}'` ;
    bulk_update_id=`echo $reverts | awk -F"^" '{print $10}'`;
	printf "\n\nInitiating Revert [ $revert_to_request_id ] For -> `echo $server $input_given_file_exec_from_repo | tr '^' ' '` ...\n" ;

	try_config_lock $machine
	[ `echo $servers_update_failed_for | grep ${reverse_ip_map[$machine]} | wc -l` -gt 0 ] && continue;          #couldnt acquire lock, skip
	
	request_time=`date +"%s"` ; 
	printf "\n%-60s" "Configuring Revert..." ;
	ssh -q $remote_user$machine "echo $author $updated_from $request_time ${reverse_ip_map[$machine]} $update_id $update_description ${options_map[$update_option]} $repo_to_update $input_given_file_exec_from_repo $bulk_update_id >> $remote_update_file | echo true" > $TEMP_SSH_STATUS_CHECK 2>/dev/null & 

	spinner $! ;

	action "" [ -s $TEMP_SSH_STATUS_CHECK ] ; 

	[ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "FAILED_TO_CONFIGURE_UPDATES" ${reverse_ip_map[$machine]}; continue; } 

	printf "\n%-60s" "Verifying Revert..." ; 

	>$TEMP_SSH_STATUS_CHECK ; 
	ssh -q $remote_user$machine "grep $update_id $remote_update_file | wc -l 2>/dev/null" > $TEMP_SSH_STATUS_CHECK 2>/dev/null ; 

	spinner $! ;

	[ -s $TEMP_SSH_STATUS_CHECK ] || action "" false ; 
	[ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ${reverse_ip_map[$machine]}; continue; }

    [ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || action "" false ;
    [ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || { update_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ${reverse_ip_map[$machine]}; continue; }
	action "" true ; 

	update_records $author $updated_from $request_time ${reverse_ip_map[$machine]} $update_id $update_description ${options_map[$update_option]} $repo_to_update $input_given_file_exec_from_repo $bulk_update_id; 
	confirm_revert_type
	cleanup $machine ; 

    done 

}

fetch_update_type () {

    echo "Production-Update Options" ;
    for options in ${!options_map[*]} ; 
    do 

	echo "$options -> ${options_map[$options]}" ;

    done ;

    printf "_\b" ; read update_option ; 
    [ ! -z $update_option ] || fetch_update_type ; 

    if [ ${options_map[$update_option]+_} ] ; then 

	case $update_option in 

	    h) printf "\n======================================= HELP ON OPTIONS =======================================\n" ; 
        echo "h) Help -> Prints This Message" ;
        #echo "0) UpdateAll -> Updates The Entire Production Setup, All Execs Are To Be Replaced After Built From Infra/Bt, Same With Configs" ; 
        #echo "1) UpdateAllProdExecs -> Updates All Production Execs From Repo" ; 
        #echo "2) UpdateAllConfigFiles -> Updates All N/w Configs/ORS Configs etc from Infra master" ; 
        #echo "3) UpdateAllScripts -> Updates All Scripts etc from Infra master" ; 
        #echo "4) UpdateExecFromRepo -> Updates Execs From Repo" ; 
        #echo "5) UpdateConfigFromRepo -> Updates Configs From Repo" ; 
        #echo "6) UpdateScriptFromRepo -> Updates Scripts From Repo" ;
		echo "7) UpdateGivenExec -> Updates A Specific Given Exec/s" ; 
		echo "8) UpdateGivenConfigFile -> Updates A Specific Given Config File/s" ;
		echo "9) UpdateGivenScript -> Updates A Specific Script/s" ;
        echo "B) BrowseUpdatesHistory -> Shows History Of The Updates Executed" ;
		echo "R) RevertTo -> You Can Browse Through The History And Revert To Any Given Request Id" ;
        echo "r) RevertLast -> You Can Browse Through The History And Revert Any Given Request Id" ;
        echo "b) BulkRevert -> You Can Browse Through The History And Bulk Revert Any Given Group Request Id" ;
		echo "q) quit -> Exits the script" ;
		printf "\n======================================================= =======================================\n" ; 
		fetch_update_type ; 
		;;
	    0|1|2|3) fetch_repo "Input Which Repo You Want To Update From [ infracore/devtrade/devmodel ]" ;
                input_given_file_exec_from_repo="ALL" ;
                fetch_servers_list ; 

		;; 
	    4) printf "Provide An Exec Or Multiple Execs Names Separated By Spaces To Be Updated From Prod Repo -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
                read -r -a input_given_file_exec_from_repo ; 

                while [ -z $input_given_file_exec_from_repo ] ; do

                    printf " Input Can't Be Empty.. Provide An Exec Or Multiple Execs Names Separated By Spaces To Be Updated From Prod Repo -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;  
                    read -r -a input_given_file_exec_from_repo ; 

                done 

                input_given_file_exec_from_repo=`echo ${input_given_file_exec_from_repo[*]} | tr ' ' '~'` ; 

                fetch_repo "Input From Which Repo, You Want to Update [ infracore/devtrade/devmodel ]" ; 
                fetch_servers_list ; 

		;; 
	    5) printf "Provide A ConfigFile Or Multiple ConfigFiles Separated By Spaces To Be Updated From Prod Repo -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
                read -r -a input_given_file_exec_from_repo ; 

                while [ -z $input_given_file_exec_from_repo ] ; do

                    printf " Input Can't Be Empty.. Provide A ConfigFile Or Multiple ConfigFiles Separated By Spaces To Be Updated From Prod Repo -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
                    read -r -a input_given_file_exec_from_repo ; 

                done 

                input_given_file_exec_from_repo=`echo ${input_given_file_exec_from_repo[*]} | tr ' ' '~'` ; 

                fetch_repo "Input From Which Repo, You Want to Update [ infracore/devtrade/devmodel ]" ; 
                fetch_servers_list ; 

		;; 

	    6) printf "Provide A Script Or Multiple Scripts Separated By Spaces To Be Updated From Prod Repo -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
                read -r -a input_given_file_exec_from_repo ; 
                input_given_file_exec_from_repo=`echo ${input_given_file_exec_from_repo[*]} | tr ' ' '~'` ; 

                while [ -z $input_given_file_exec_from_repo ] ; do

                    printf " Input Can't Be Empty.. Provide A Script Or Multiple Scripts Separated By Spaces To Be Updated From Prod Repo -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;
                    read -r -a input_given_file_exec_from_repo ; 

                done 

                fetch_repo "Input From Which Repo, You Want to Update [ infracore/devtrade/devmodel ]" ; 
                fetch_servers_list ; 

                ;;

	    7) printf "Provide A Local Exec Or Multiple Execs Separated By Spaces To Be Synced  -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;
		read -r -a input_specific_exec_or_config_file ; 

                while [ -z $input_specific_exec_or_config_file ] ; do

                    printf " Input Can't Be Empty.. Provide A Local Exec Or Multiple Execs Separated By Spaces To Be Synced  -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
                    read -r -a input_specific_exec_or_config_file; 

                done 

                printf "Checking If All Files Exists... Not Empty...And Readable...\n" ;

                for input_file in `echo ${input_specific_exec_or_config_file[*]}` ; do 
                    action "Testing $input_file ..." [ -e $input_file -a -f $input_file -a -s $input_file -a -r $input_file ] ; 
                    [ -f $input_file -a -s $input_file -a -r $input_file ] || script_failure "[ERROR] -> FILE_VALIDATION_TESTS_FAILED" ; 
                done 

                input_specific_exec_or_config_file=`echo ${input_specific_exec_or_config_file[*]} | tr ' ' '~'` ;   

                fetch_branch "Please Enter The Branch Name From Which The Exec/Execs Was/Were Built, " ; 

                fetch_servers_list ; 

		;; 
	    8) printf "Provide A Local Config or Multiple Configs Separated By Spaces To Be Synced -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
		read -r -a input_specific_exec_or_config_file ; 

                while [ -z $input_specific_exec_or_config_file ] ; do

                    printf "Input Can't Be Empty.. Provide A Local Config or Multiple Configs Separated By Spaces To Be Synced -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;
                    read -r -a input_specific_exec_or_config_file; 

                done 

                printf "Checking If All Files Exists... Not Empty...And Readable...\n" ;

                for input_file in `echo ${input_specific_exec_or_config_file[*]}` ; do 
                    action "Testing $input_file ..." [ -e $input_file -a -f $input_file -a -s $input_file -a -r $input_file ] ; 
                    [ -f $input_file -a -s $input_file -a -r $input_file ] || script_failure " [ERROR] -> FILE_VALIDATION_TESTS_FAILED" ; 
                done 

                input_specific_exec_or_config_file=`echo ${input_specific_exec_or_config_file[*]} | tr ' ' '~'` ; 

                fetch_branch "Please Enter The Branch Name From Which The Exec Was Built, " ; 

                fetch_servers_list ; 

		;; 

	    9) printf "Provide A Local Script or Multiple Scripts Separated By Spaces To Be Synced -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
		read -r -a input_specific_exec_or_config_file ; 

                while [ -z $input_specific_exec_or_config_file ] ; do

                    printf "Input Can't Be Empty.. Provide A Local Script or Multiple Scripts Separated By Spaces To Be Synced -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
                    read -r -a input_specific_exec_or_config_file; 

                done 

                [ ! -z $input_specific_exec_or_config_file ] || script_failure "[ERROR] -> NO_INPUT_RECEIVED" ;

                printf "Checking If All Files Exists... Not Empty...And Readable...\n" ;

                for input_file in `echo ${input_specific_exec_or_config_file[*]}` ; do 
                    action "Testing $input_file ..." [ -e $input_file -a -f $input_file -a -s $input_file -a -r $input_file ] ; 
                    [ -f $input_file -a -s $input_file -a -r $input_file ] || script_failure " [ERROR] -> FILE_VALIDATION_TESTS_FAILED" ; 
                done 

                input_specific_exec_or_config_file=`echo ${input_specific_exec_or_config_file[*]} | tr ' ' '~'` ; 

                fetch_branch "Please Enter The Branch Name From Which The Exec Was Built, " ; 

                fetch_servers_list ; 

		;;

	    M) if [ "$updated_from" == "sdv-ny4-srv11" ] ; then
	
			printf "Please Enter the products separated by spaces -> ____________\b\b\b\b\b\b\b\b\b\b\b\b " ;
			read -r -a input_products_or_machines;
			while [ -z $input_products_or_machines ] ; do
		
			    printf "Input Can't Be Empty.. Please Enter the products separated by spaces -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
			    read -r -a input_products_or_machines;
			done
			try_product_machine_access_lock
			for input_product in `echo ${input_products_or_machines[*]}` ; 
			do
			    display_machine_for_product $input_product
			done
			printf "\n";

			#only lock acquired is prod_mach lock on NY11
			#the script exits here. So clean all lock
			if [ "$prod_mach_lock_acquired" == "true" ] ; then
			    rm -f $product_machine_lockfile_access
			    prod_mach_lock_acquired="false";
			    printf "Removed product_machine file Lock removed \n";
			fi
		    else
			script_failure "[ERROR] -> Options S, M & U can be used only from NY11" ;
		    fi
    
    	;;
	    S) if [ "$updated_from" == "sdv-ny4-srv11" ] ; then
			printf "Please Enter the machines separated by spaces -> ____________\b\b\b\b\b\b\b\b\b\b\b\b " ;
			read -r -a input_products_or_machines;
			while [ -z $input_products_or_machines ] ; do
	
			    printf "Input Can't Be Empty.. Please Enter the products separated by spaces -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
			    read -r -a input_products_or_machines;
			done
			try_product_machine_access_lock
			for input_machine in `echo ${input_products_or_machines[*]}` ; 
			do
			    get_all_products_for_machine $input_machine
			done
			printf "\n";

				#only lock acquired is prod_mach lock on NY11
				#the script exits here. So clean all lock
			if [ "$prod_mach_lock_acquired" == "true" ] ; then
			    rm -f $product_machine_lockfile_access
			    prod_mach_lock_acquired="false";
			    printf "Removed product_machine file Lock removed \n";
			fi
			else
			script_failure "[ERROR] -> Options S, M & U can be used only from NY11" ;
			fi
		;;
	    
	    U) if [ "$updated_from" == "sdv-ny4-srv11" ] ; then
			printf "Please Enter the product for which the machine needs to be changed -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;
			input_product_=""			
			read input_product_;
			
			while [ -z $input_product_ ] ; do
	
			    printf "Input Can't Be Empty.. Please Enter the product for which the machine needs to be changed -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
			    read input_product_;
			done
	    	#display machine for this product and ask if change is needed.
			try_product_machine_access_lock
			display_machine_for_product $input_product_
			if [ "$prod_mach_lock_acquired" == "true" ] ; then
			    rm -f $product_machine_lockfile_access
			    prod_mach_lock_acquired="false";
			fi

			printf "Do you want to change? (y/n) Press q to exit -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;
			option_="";		
			read option_
			if [ "$option_" == "y" ] || [ "$option_" == "Y" ] ; then
			    printf "Enter the new machine for $input_product_ -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
			    new_machine_="";
			    read new_machine_;
			    while [ -z $new_machine_ ] ; do
				printf "Input Can't Be Empty.. Enter the new machine for $input_product_ -> ____________\b\b\b\b\b\b\b\b\b\b\b\b"
				read new_machine_;
			    done
			    update_product_machine_map $input_product_ $new_machine_
			else
			    script_failure "THANK_YOU_FOR_USING_SCRIPT !!" ;	
			fi
			else
				script_failure "[ERROR] -> Options S, M & U can be used only from NY11" ;
			fi
		;;

        B|R|r|b) 

		if [ "$update_option" == "R"  ] || [ "$update_option" == "r"  ] || [ "$update_option" == "b"  ]; then 
                    printf "Press Y/y If You Have The RequestID To Which You Want To Revert To Or Any Other Key... -> _\b" ; 
                    read do_we_have_request_id ; 

                    if [ "$do_we_have_request_id" == "Y" ] || [ "$do_we_have_request_id" == "y" ] ; then 

			printf "Please Enter The Request Id To Which You Want To Revert To -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
			read revert_to_request_id ; 

			while [ -z $revert_to_request_id ] ; do

			    printf "Revert Request Id Can't Be Empty...Please Enter The Request Id To Which You Want To Revert To -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
			    read revert_to_request_id ;

			done 

			revert_updates $revert_to_request_id ;  
					#revert is complete, don't need to proceed further
					exit; 

                    fi 

		fi 
		
		printf "Browse By ServerMnemonics [ Empty For All ] -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
		read browse_by_server ; 

		[ ! -z $browse_by_server ] || browse_by_server="" ;
 
		printf "Browse By ExecName [ Empty For All ] -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
		read browse_by_execname ; 

		[ ! -z $browse_by_execname ] || browse_by_execname="" ; 

		printf "Browse By UpdateId [ Empty For All ] -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
		read browse_by_updateid ;

		[ ! -z $browse_by_updateid ] || browse_by_updateid="" ; 

		if [ "$browse_by_server" == "INVALID" ] && [ "$browse_by_execname" == "INVALID" ] && [ "$browse_by_updateid" == "INVALID" ] ; then

                    browse_by_server="" ; 
                    browse_by_execname="" ;
                    browse_by_updateid="" ;

		fi 

		echo $browse_by_server " " $browse_by_execname " " $browse_by_updateid ; 

		clear ; echo "Listing Last 25 Updates Done..." ; 
		printf "|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|\n" "   RequestId   " "     Server    " "     Author    " "      From     " "   UpdateTime  " "    Branch     " "  Description  " "    UpdateOptionUsed     " "             ExecsUpdated               " "             BulkUpdateId               " ; 
		printf "|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|\n" "_______________" "_______________" "_______________" "_______________" "_______________" "_______________" "_________________________" "_________________________" "_____________________________________________" "_____________________________________________" ; 

        revert_color=$'\033[31;1m' ; 

		revert_color=$'\033[31;1m'
		color_end=$'\033[0m'

		ssh $remote_user$database_server "cat $database_update_records" | grep "$browse_by_server" | grep "$browse_by_execname" | grep "$browse_by_updateid" | tail -25 | awk '{ if ( $8 == "RevertTo" ) { printf ("\033[31;1m|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|\033[0m\n|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,"_______________","_______________","_______________","_______________","_______________","_______________","_________________________","_________________________","_____________________________________________","_________________________")} else { { printf ("|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|\n|%-15s|%-15s|%-15s|%-15s|%-15s|%-15s|%-25s|%-25s|%-45s|%-25s|\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10,"_______________","_______________","_______________","_______________","_______________","_______________","_________________________","_________________________","_____________________________________________","_________________________")} } }' ; 

		if [ "$update_option" == "R" ] || [ "$update_option" == "r" ] || [ "$update_option" == "b"  ]; then 

                    printf "\n\n\nPlease Enter The Request Id To Which You Want To Revert To -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
                    read revert_to_request_id ; 

                    while [ -z $revert_to_request_id ] ; do

			printf "Revert Request Id Can't Be Empty...Please Enter The Request Id To Which You Want To Revert To -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
			read revert_to_request_id ;

                    done 

                    revert_updates $revert_to_request_id ;  

		else 

                    read dummy ; 
                    exit ;
		    
		fi 

		;; 

	    q) exit ; 
		;; 
	    *) echo "Invalid Option, Please Try Again..." ; 
		fetch_update_type ; 
		;; 

	esac 

	else 
	echo "Invalid Option, Please Try Again..." ; 
	fetch_update_type ; 
	fi 

}



confirm_update_type () {
	[ ! -z $servers_update_failed_for ] &&  printf "\n Updates failed for $servers_update_failed_for . Please retry. \n ";
}

update_records () {

#    printf "\n===== UpdateRequestDetails ===== \n\nAUTHOR -> $1\nUpdatedFrom -> $2\nRequestTime -> `date -d @$3`\nMachine -> $4\nUpdateDescription -> $6\nUpdatedWithOption -> $7\nRepoUsed -> `echo $8|tr '~' ' '`\nExecName -> $9\n" | /bin/mail -s "ProductionUpdateRequestLodged - $5" -r "pengine@dvcapital.com" "ravi.parikh@tworoads.co.in" ;


    echo $* | awk '{print "Request at "$4" by "$1": "$6" "$5" "$9" "$10"" }' > $temp_slack_update_requests_file ;
    scp $temp_slack_update_requests_file dvcinfra@10.23.5.67:/tmp/ ; 
    ssh $remote_user$database_server "$slack_notification_exec $update_channel FILE $temp_slack_update_requests_file" ; 

    : '    printf "%-60s" "Checking Database Server Accessibility..." ; 

	>$TEMP_SSH_STATUS_CHECK ; 
	ssh -q $remote_user$database_server 'echo true' > $TEMP_SSH_STATUS_CHECK & >/dev/null 

	spinner $! ; 

	action "" [ -s $TEMP_SSH_STATUS_CHECK ] ;

	[ -s $TEMP_SSH_STATUS_CHECK ] || echo "DATABSE_SERVER_NOT_ACCESSIBLE_REQUEST_WILL_NOT_BE_RECORDED" ;
	[ -s $TEMP_SSH_STATUS_CHECK ] || return ; 
	'
    >$TEMP_SSH_STATUS_CHECK ; 
    
    printf "%-60s" "Updating Database Records..." ; 
    ssh -q $remote_user$database_server "echo $* >> $database_request_records" & >/dev/null 2>/dev/null ; 
    spinner $! ; 
    
    action "" true ; 

}

fetch_author_name () {

    printf "$1, q to exit -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ;
    read -e author ;

    if [ "$author" == "q" ] ; then

	script_failure "THANK_YOU_FOR_USING_SCRIPT!!" ;

    fi

    if [ `echo $author | awk '{print NF}'` -ne 1 ] ; then

	fetch_author_name "Invalid Input, Please Provide Your Single Word First Name...Try Again..." ;

    fi

    if [ `echo $author | grep "guest" | wc -l` -ne 1 ] ; then
       
       if [ `echo $authorized_author_list | grep "$author" | wc -l` -ne 1 ] ; then
  
         fetch_author_name "You Are Not Authorized To Update Production Setup...Try Again...Or use guest_yourname"  ;

       fi
    fi

}

fetch_misc_details () {
    fetch_author_name "Please Enter Your First Name ( all in small letters ) " ; 

    printf "Please Provide One Word Update Description -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
    read -e update_description ; 

    while [ `echo $update_description | awk '{print NF}'` -ne 1 ] 
    do
	
	printf "You Entered `echo $update_description | awk '{print NF}'` Words, Just One Plaese.. ( use _ to club words ) -> ____________\b\b\b\b\b\b\b\b\b\b\b\b" ; 
	read -e update_description ; 

    done

    >$TEMP_SSH_STATUS_CHECK ; 

    [ -f $TEMP_SSH_STATUS_CHECK ] || script_failure "[ERROR] -> DON'T_HAVE_PERMISSIONS_TO_CREATE_$TEMP_SSH_STATUS_CHECK" ; 

}

try_config_lock () {
    machine=$1
    printf "%-60s" "Checking $server Accessibility For IP -> $machine ..." ;

    >$TEMP_SSH_STATUS_CHECK ; 
    ssh -q $remote_user$machine 'echo true' > $TEMP_SSH_STATUS_CHECK & 

    spinner $! ; 

    action "" [ -s $TEMP_SSH_STATUS_CHECK ] ;

[ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "[ERROR] -> SERVER $machine NOT_ACCESSIBLE" ${reverse_ip_map[$machine]} ; return; } 

    >$TEMP_SSH_STATUS_CHECK ; 
    printf "%-60s" "Acquiring Lock..." ;
    ssh -q $remote_user$machine /bin/bash << \EOF > $TEMP_SSH_STATUS_CHECK &
    remote_lockfile_access="/spare/local/.update_production_setup_lock.lock";
    for attempt in {1..10} ; 
    do

	if [ ! -e $remote_lockfile_access ] ; then 
	    touch $remote_lockfile_access ;  break ;  
	else 
	    if [ `expr $(date +%s) - $(date +%s -r $remote_lockfile_access)` -gt "30" ] ; then
		rm $remote_lockfile_access
	    fi 
	fi ; 
	sleep 1 ;
    done ;
    echo $attempt
EOF
    spinner $! ;        
    attempt=`cat $TEMP_SSH_STATUS_CHECK` ;
    action "" [ "$attempt" != "10" ] ; 

    [ "$attempt" != "10" ] || { update_failure "[ERROR] -> FAILED_TO_ACQUIRE_LOCK" ${reverse_ip_map[$machine]}; return; }  
    lock_acquired="true" ; 
    >$TEMP_SSH_STATUS_CHECK ; 

    }

    update_product_machine_map () {
	input_product_=$1
	new_machine_=$2
	this_request_="$input_product_~$new_machine_" ;
	fetch_misc_details		#fetches details like author, username, update description

	machine="10.23.5.67" ; # this request only goes to NY11
	try_config_lock $machine
	request_time=`date +"%s"` ; 
	printf "\n%-60s" "Configuring Updates..." ; 
	ssh -q $remote_user$machine "echo $author $updated_from $request_time NY11 $update_id $update_description ${options_map[$update_option]} NOREPO $this_request_ >> $remote_update_file | echo true" > $TEMP_SSH_STATUS_CHECK 2>/dev/null & 

	spinner $! ;

	action "" [ -s $TEMP_SSH_STATUS_CHECK ] ; 

	[ -s $TEMP_SSH_STATUS_CHECK ] || script_failure "[ERROR] -> FAILED_TO_CONFIGURE_UPDATES" ; 

	printf "\n%-60s" "Verifying Updates..." ; 

	>$TEMP_SSH_STATUS_CHECK ; 
	ssh -q $remote_user$machine "grep $update_id $remote_update_file | wc -l 2>/dev/null" > $TEMP_SSH_STATUS_CHECK 2>/dev/null ; 

	spinner $! ;

	[ -s $TEMP_SSH_STATUS_CHECK ] || action "" false ; 
	[ -s $TEMP_SSH_STATUS_CHECK ] || script_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ; 

	[ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || action "" false ; 
	[ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || script_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ;

	action "" true ; 

	update_records $author $updated_from $request_time NY11 $update_id $update_description ${options_map[$update_option]} NOREPO $this_request_ ; 
	cleanup $machine ; 
    }

    fetch_servers_list () {

	fetch_misc_details	#fetches details like username, update description

	echo "Please Input Servers List You Want To Modify, Please Enter Space Separated Mnemonics," ; 
	echo "Available Servers List : `echo ${!servers_map[*]} | tr ' ' '\n' | sort | tr '\n' ' '` " ; printf "\n" ;
	printf "_____\b\b\b\b\b" ;

	read -r -a servers_list ;

	#special case handling for update to all servers available
	servers_update_failed_for="";
	[ ! "$servers_list" == "ALLSERVERS" ] || servers_list=`echo ${!servers_map[*]} | tr ' ' '\n' | grep -v "ALL" | grep -v "HK" | sort | tr '\n' ' '` ; 
	
    bulk_update_id=`date +"%s%N"` ;
    
	for server in `echo ${servers_list[*]}` ; do

	    if [ ${servers_map[$server]+_} ] ; then 

		for machine in `echo ${servers_map[$server]}` ; do 
			#generate a new update id per exec-list and server combination : easy reverts
			update_id=`date +"%s%N"` ;
	      	try_config_lock $machine
			[ `echo $servers_update_failed_for | grep ${reverse_ip_map[$machine]} | wc -l` -gt 0 ] && continue;          #couldnt acquire lock, skip
		    case $update_option in 
			1|2|3|4|5|6)
			    
			    request_time=`date +"%s"` ; 
			    printf "\n%-60s" "Configuring Updates..." ; 
  			    ssh -q $remote_user$machine "echo $author $updated_from $request_time ${reverse_ip_map[$machine]} $update_id $update_description ${options_map[$update_option]} $repo_to_update $input_given_file_exec_from_repo $bulk_update_id >> $remote_update_file | echo true" > $TEMP_SSH_STATUS_CHECK 2>/dev/null & 

			    spinner $! ;
              		    action "" [ -s $TEMP_SSH_STATUS_CHECK ] ; 

			    [ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "[ERROR] -> FAILED_TO_CONFIGURE_UPDATES" ${reverse_ip_map[$machine]}; continue; }

			    printf "\n%-60s" "Verifying Updates..." ; 

			    >$TEMP_SSH_STATUS_CHECK ; 
			    ssh -q $remote_user$machine "grep $update_id $remote_update_file | wc -l 2>/dev/null" > $TEMP_SSH_STATUS_CHECK 2>/dev/null ; 
			    spinner $! ;
			    [ -s $TEMP_SSH_STATUS_CHECK ] || action "" false ; 
			    [ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ${reverse_ip_map[$machine]}; continue; } 
			    [ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || action "" false ; 
			    [ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || { update_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ${reverse_ip_map[$machine]}; continue; }
			    action "" true ; 
			    update_records $author $updated_from $request_time ${reverse_ip_map[$machine]} $update_id $update_description ${options_map[$update_option]} $repo_to_update $input_given_file_exec_from_repo $bulk_update_id ; 

			    cleanup $machine ; 


			    ;; 

			7|8|9) 

                            multiple_files=`echo $input_specific_exec_or_config_file | tr '~' ' '` ; 
                            current_file_md5sum=`/usr/bin/md5sum $multiple_files | awk '{print $1}' | tr '\n' ' '` ;

                            multiple_files_with_spaces="" ;
                            multiple_files_with_update_id="" ;
                            mf="" ;
                            last_files=`echo $multiple_files | awk '{print $NF}'` ; 

                            for files in `echo $multiple_files` ; do
				#create a temporay copy, upload and delete
                                file_path_with_update_id=/tmp/`echo $files | awk -F"/" '{print $NF}'`"#"$update_id;
				cp -f $files $file_path_with_update_id ;

				if [ "$files" == "$last_files" ] ; then 

				    multiple_files_with_spaces=$multiple_files_with_spaces`echo $files | awk -F"/" '{print $NF}'`; 
				    multiple_files_with_update_id=$multiple_files_with_update_id$file_path_with_update_id ;
                    mf=$mf$file_path_with_update_id".gz";
                else 
 

				    multiple_files_with_spaces=$multiple_files_with_spaces`echo $files | awk -F"/" '{print $NF}'`"~" ; 
				    multiple_files_with_update_id=$multiple_files_with_update_id$file_path_with_update_id" " ; 
                    mf=$mf$file_path_with_update_id".gz"" ";

				fi 

                            done 
                            gzip -f $multiple_files_with_update_id;
                            printf "%-60s" "Uploading Files..." ;
                            scp -q $mf $remote_user$machine:$REMOTE_POOL_FOR_UPDATES 2>/dev/null >/dev/null ;
                            spinner $! ; 
                            action "" true ; 
                            
                        #files uploaded, safe to delete the # appended files
                            for files in `echo $multiple_files` ; do
				rm -f /tmp/`echo $files | awk -F"/" '{print $NF}'`"#"$update_id".gz" ;
                            done

                            > $TEMP_SSH_STATUS_CHECK ; 
                            printf "%-60s" "Verifying MD5SUM..." ; 
                            ssh -q $remote_user$machine /bin/bash << EOF > $TEMP_SSH_STATUS_CHECK &
                            /usr/bin/md5sum $REMOTE_POOL_FOR_UPDATES*$update_id".gz" | awk '{print $1}' | tr '\n' ' ' 2>/dev/null
                            
EOF
                            spinner $! ; 
                            status=`cat $TEMP_SSH_STATUS_CHECK` ; 
                            if [ ! -s $TEMP_SSH_STATUS_CHECK ] 
                            then 

				action "" false ;
				update_failure  "REMOTE_UPDATE_POOL_DIRECTORY_DOESN'T_EXISTS_OR_BAD_PERMISSIONS" ${reverse_ip_map[$machine]}; continue; 

                            else 

				status=`cat $TEMP_SSH_STATUS_CHECK` ;
				action "" [ "$status" != "$current_file_md5sum" ] ;

                            fi 

                            request_time=`date +"%s"` ; 
                            printf "\n%-60s" "Configuring Updates..." ; 
                            ssh -q $remote_user$machine  /bin/bash << EOF > $TEMP_SSH_STATUS_CHECK 2>/dev/null & 
                            echo $author $updated_from $request_time ${reverse_ip_map[$machine]} $update_id $update_description ${options_map[$update_option]} $repo_to_update $multiple_files_with_spaces $bulk_update_id >> $remote_update_file | echo true
EOF
                            spinner $! ;

                            action "" [ -s $TEMP_SSH_STATUS_CHECK ] ; 

                            [ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "FAILED_TO_CONFIGURE_UPDATES" ${reverse_ip_map[$machine]}; continue; } 

                            printf "\n%-60s" "Verifying Updates..." ; 

                            >$TEMP_SSH_STATUS_CHECK ; 
                        ssh -q $remote_user$machine /bin/bash << EOF > $TEMP_SSH_STATUS_CHECK 2>/dev/null &
                          grep $update_id $remote_update_file | wc -l 2>/dev/null 
EOF
                        spinner $! ;

                        [ -s $TEMP_SSH_STATUS_CHECK ] || action "" false ; 
                        [ -s $TEMP_SSH_STATUS_CHECK ] || { update_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ${reverse_ip_map[$machine]}; continue; } 

                        [ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || action "" false ; 
                        [ `cat $TEMP_SSH_STATUS_CHECK` -eq 1 ] || { update_failure "[ERROR] -> UPDATE_FAILED_LIKELY_PERMISSION_ISSUES_WITH_REMOTE_FILE : $remote_update_file" ${reverse_ip_map[$machine]}; continue; }

                        action "" true ; 

                        update_records $author $updated_from $request_time ${reverse_ip_map[$machine]} $update_id $update_description ${options_map[$update_option]} $repo_to_update $input_specific_exec_or_config_file $bulk_update_id ; 

                        cleanup $machine ; 

			;; 

			*) script_failure "CURRENTLY_NOT_SUPPORTED" ; 
			
                        ;; 

			esac 

			done ; 


			else 
			update_failure "[ERROR] -> Machine Mnemonic Doesn't Exist" ${reverse_ip_map[$machine]}; continue ;
			fi 

			done 

			confirm_update_type ; 

}

clear ; 
initialize ;
fetch_update_type ;
