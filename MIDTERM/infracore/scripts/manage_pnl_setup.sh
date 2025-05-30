#! /bin/bash

trade_type="";
YYYYMMDD="";
PULL_SERVER_MANAGE_SCRIPT="$HOME/infracore/scripts/manage_pushdelta_setup.sh"
TAGWISE_PNL_CONFIG="/home/pengine/prod/live_configs/tagwise_pnl_config"
declare -A server_hostname_map ;
declare -a mft_hft_servers=(IND12);

TAG_PNL_DIR="/spare/local/logs/pnl_data/hft/tag_pnl/";
TAG_PNL_SCRIPT="/home/pengine/prod/live_scripts/show_pnl_generic_tagwise.pl";
TAG_PNL_FILE="${TAG_PNL_DIR}tagwise_pnl.txt";
TAG_PNL_ERR_FILE="${TAG_PNL_DIR}pnl_tagwise_err.log";

TI_PNL_FILE="${TAG_PNL_DIR}ti_pnl.txt";
TI_PNL_ERR_FILE="${TAG_PNL_DIR}pnl_ti_err.log";

server_hostname_map[FR15]="sdv-fr2-srv15";  #10.23.102.55"
server_hostname_map[FR16]="sdv-fr2-srv16";   #"10.23.102.56"
server_hostname_map[FR13]="sdv-fr2-srv13";     #10.23.200.53
server_hostname_map[FR14]="sdv-fr2-srv14"; #10.23.200.54
server_hostname_map[CHI15]="sdv-chi-srv15"; #10.23.82.55
server_hostname_map[CHI16]="sdv-chi-srv16"; #10.23.82.56
server_hostname_map[CHI13]="sdv-chi-srv13"; # 10.23.82.53
server_hostname_map[CHI14]="sdv-chi-srv14"; # 10.23.82.54
server_hostname_map[TOR11]="sdv-tor-srv11"; #10.23.182.51
server_hostname_map[TOR12]="sdv-tor-srv12"; #10.23.182.52
server_hostname_map[BSL11]="sdv-bsl-srv11" #10.23.52.51
server_hostname_map[BSL12]="sdv-bsl-srv12";
server_hostname_map[BSL13]="sdv-bsl-srv13";
server_hostname_map[BMF11]="sdv-bmf-srv11"; #10.220.65.35
server_hostname_map[BMF12]="sdv-bmf-srv12"; #10.220.65.34
server_hostname_map[BMF15]="sdv-bmf-srv15"; #10.220.65.36
server_hostname_map[BMF13]="sdv-bmf-srv13"; #10.220.65.33
server_hostname_map[BMF14]="sdv-bmf-srv14"; #10.220.65.38
server_hostname_map[MOS11]="sdv-mos-srv11"; #172.18.244.107
server_hostname_map[MOS12]="sdv-mos-srv12"; #10.23.241.2
server_hostname_map[HK11]="SDV-HK-SRV11"; #10.152.224.145
server_hostname_map[HK12]="SDV-HK-SRV12"; #10.152.224.146
server_hostname_map[OSE12]="sdv-ose-srv12"; #10.134.73.211
server_hostname_map[OSE11]="sdv-ose-srv11"; #10.134.73.212
server_hostname_map[OSE13]="sdv-ose-srv13"; #10.134.73.213
server_hostname_map[OSE14]="sdv-ose-srv14"; #10.134.73.214
server_hostname_map[CFE11]="sdv-cfe-srv11"; #10.23.74.61
server_hostname_map[CFE12]="sdv-cfe-srv12"; #10.23.74.62
server_hostname_map[CFE13]="sdv-cfe-srv13"; #10.23.74.63
server_hostname_map[IND12]="sdv-ind-srv12"; #10.23.115.62
server_hostname_map[IND11]="sdv-ind-srv11"; #10.23.115.61
server_hostname_map[IND13]="sdv-ind-srv13"; #10.23.115.63
server_hostname_map[ASX11]="SDV-ASX-SRV11"; #10.23.43.51
server_hostname_map[ASX12]="SDV-ASX-SRV12"; #10.23.43.52
server_hostname_map[SGX11]="sdv-sgx-srv11"; #10.23.26.51
server_hostname_map[SGX12]="sdv-sgx-srv12"; #10.23.26.52


start () {

  if [ "$#" -eq 1 ] ; then
    trade_type=$1
    if [[ "$trade_type" != "H"  &&   "$trade_type" != "M" &&   "$trade_type" != "N" ]] ; then
      echo "invalid trade_type. Enter H(hft) or M(mft) or N(for NSE)";
      exit 0;
    fi
  else
    echo "Enter a trade_type. start H(hft) or start M(mft) or start N(NSE)";
    exit 0;
  fi

  time_now_=`date +"%H%M%S"` ;
	pnl_start_time=`grep STARTTIME $TAGWISE_PNL_CONFIG | awk '{print $2}'`;
  #if restart is after 2152 (pnl calculations start time) before 00:00
  #trim leading 0 in time_now_ as [[]] interprets them as octal numbers
  if [[ "${time_now_#0}" -gt "$pnl_start_time"  &&  "${time_now_#0}" -lt "235900" ]] ; then
    YYYYMMDD=$(date --date='tomorrow' +"%Y%m%d")
  else
    YYYYMMDD=$(date "+%Y%m%d");
  fi

  trade_type_path="hft";
  if [ "$trade_type" == "M" ] ; then
    trade_type_path="mtt";
  fi

  #check if already running, notify user and dont create another instance
  process_name="calc_ors_pnl.pl C "$trade_type
	if  [ "$trade_type" == "N" ] ; then
		process_name="show_pnls.pl C H"
	fi
	
  is_alive=`ps -ef |grep "$process_name" | grep perl | awk '{print $2}' | wc -l`;
  if [ "$is_alive" -gt 0 ] ; then
    echo "Already running";
    exit 0;
  fi

  #Check if Already running, don't create annother instance Tagwise_pnl

	if  [ "$trade_type" == "H" ] ; then
		process_name_1="show_pnl_generic_tagwise.pl C "$trade_type
		is_alive=`ps -ef |grep "$process_name_1" | grep perl | awk '{print $2}' | wc -l`;
		if [ "$is_alive" -gt 0 ] ; then
			echo "Already running";
			exit 0;
		fi
	fi
	
	
  #all good to start pnl calculations
  #run in background as child of init
  if [ "$trade_type" == "N" ] ; then
    nohup perl /home/dvcinfra/LiveExec/scripts/show_pnls.pl 'C' 'H' $YYYYMMDD T  \
    			> /spare/local/logs/pnl_data/$trade_type_path/nse_tagwise_pnls.txt 2> /spare/local/logs/pnl_data/$trade_type_path/nse_pnl_log &
    nohup perl /home/dvcinfra/LiveExec/scripts/show_pnls.pl 'C' 'H' $YYYYMMDD Q  \
    			> /spare/local/logs/pnl_data/$trade_type_path/nse_querywise_pnls.txt 2> /spare/local/logs/pnl_data/$trade_type_path/nse_pnl_log &
  else
	nohup perl /home/dvcinfra/LiveExec/scripts/calc_ors_pnl.pl 'C' $trade_type $YYYYMMDD \
                > /spare/local/logs/pnl_data/$trade_type_path/pnls.txt 2>>/spare/local/logs/pnl_data/$trade_type_path/pnl_log &
        nohup perl "$TAG_PNL_SCRIPT" 'C' 'H' "$YYYYMMDD"  'TT' N >"$TAG_PNL_FILE" 2>"$TAG_PNL_ERR_FILE" &
        nohup perl "$TAG_PNL_SCRIPT" 'C' 'H' "$YYYYMMDD"  'TI' N >"$TI_PNL_FILE" 2>"$TI_PNL_ERR_FILE" &
  fi
  
  is_alive=`ps -ef |grep "$process_name" | grep perl | awk '{print $2}' | wc -l`;
  if [ "$is_alive" -gt 0 ] ; then
    echo "Successfully started $process_name";
  fi
  is_alive=`ps -ef |grep "$process_name_1" | grep perl | awk '{print $2}' | wc -l`;
  if [ "$is_alive" -gt 0 ] ; then
     echo "Successfully started $process_name_1";
  fi
}

stop () {
  if [ "$#" -eq 1 ] ; then
    trade_type=$1
    if [[ "$trade_type" != "H"  &&   "$trade_type" != "M" &&   "$trade_type" != "N" ]] ; then
      echo "invalid trade_type. Enter H(hft) or M(mft) or N(NSE)";
      exit 0;
    fi
  else
    echo "Enter a trade_type. start H(hft) or start M(mft) or start N(NSE)";
    exit 0;
  fi
  process_name="calc_ors_pnl.pl C "$trade_type
  if  [ "$trade_type" == "N" ] ; then
	 process_name="show_pnls.pl C H"
  fi
  pid=`ps -ef | grep "$process_name" | grep perl | awk '{print $2}'`
  for id in $pid; do
	kill $id
  done
  #check if killed
  is_alive=`ps -ef |grep "$process_name" | grep perl | awk '{print $2}' | wc -l`;
  pid=`ps -ef | grep "$process_name" | grep perl | awk '{print $2}'`
  if [ "$is_alive" -gt 0 ] ; then
    for id in $pid; do
		kill -9 $id
	done
	echo "Sent -9 kill.";
  else
    echo "Successfully stopped $process_name";
  fi
 #Tagwise Pnl management
	if  [ "$trade_type" == "H" ] ; then
		process_name="show_pnl_generic_tagwise.pl C "$trade_type
		pid=`ps -ef | grep "$process_name" | grep perl | awk '{print $2}'`
		for id in $pid; do
			kill $id
		done
		#check if killed
		is_alive=`ps -ef |grep "$process_name" | grep perl | awk '{print $2}' | wc -l`;
		pid=`ps -ef | grep "$process_name" | grep perl | awk '{print $2}'`
		if [ "$is_alive" -gt 0 ] ; then
			for id in $pid; do
				kill -9 $id
			done
			echo "Sent -9 kill.";
		else
			echo "Successfully stopped $process_name";
		fi
	fi
}

delete_delta () {
  if [ "$#" -eq 2 ] ; then
    trade_type=$1
    shift
    if [[ "$trade_type" != "H"  &&   "$trade_type" != "M" ]] ; then
      echo "invalid trade_type. Enter H or M";
      exit 0;
    fi
    #parse the server given
    server=$1
    if [ ${server_hostname_map[$server]+_} ] ; then
      echo "Deleting all deltas for host ${server_hostname_map[$server]}"
    else
      echo "Incorrect server_mnemonic. Please check."
      exit 0;
    fi
  else
    echo "Usage: $0 delete H/M Server_menomics ";
    echo "Available Server menomics " ${!server_hostname_map[*]}
    exit 0;
  fi

  #set the delta dir path
  trade_type_path="hft";
  if [ "$trade_type" == "M" ] ; then
    trade_type_path="mtt";
  fi

  #get all deltas for this server_mnemonic
  delta_dir="/spare/local/logs/pnl_data/$trade_type_path/delta_files/";
  echo $delta_dir "${server_hostname_map[$server]}"
  FILES=(`find "$delta_dir" -type f -name "*${server_hostname_map[$server]}*"`)
  if [ ! -z "$FILES" ] ; then
    for i in "${FILES[@]}"
    do
        #echo "$i"
        rm -f "$i"
    done
  fi

  #check if deleted
  delta_present=`find "$delta_dir" -type f -name  "*${server_hostname_map[$server]}*" | wc -l`
  if [ "$delta_present" -gt 0 ] ; then
    echo "error deleting files";
  else
    echo "all files Successfully deleted"
  fi
}

#IMPORTANT: incase of a server which has trade files for both hft and mft(IND12)
#if reset is requested, delete and restart operation needs to be done for both mft and hft pnls script

reset_pnl () {
  if [ "$#" -ne 2 ]; then
       echo "Invalid input. Enter server name"
       exit
  else
	#stop the dvc_pushdelta service on server in $2
    $PULL_SERVER_MANAGE_SCRIPT stop $2
  fi
  
  #delete the delta files for server & restart the pnl setup on NY11: calc_ors_pnl/show_pnls
  if [[ "${mft_hft_servers[@]}" =~ "$2" ]]; then
  	delete_delta M $2
  	delete_delta H $2
  	restart_pnl_setup M
  	restart_pnl_setup N
  else
  	delete_delta H $2
  	restart_pnl_setup $1
  fi

  #start the dvc_pushdelta service on server in $2
  $PULL_SERVER_MANAGE_SCRIPT start $2
}

restart_pnl_setup() {
  stop $@
  start $@
}

# See how we were called.

case "$1" in
START|start)
#start H or start M
shift
start $@

;;
STOP|stop)

#stop H or stop M
shift
stop $@

;;
RESTART|restart)
shift
restart_pnl_setup $@
;;

delete|Delete)
shift
delete_delta $@
;;

reset|RESET)
shift
reset_pnl $@
;;

*)
echo "Usage: $0 {start H/M/N | stop H/M/N |restart H/M/N | delete H/M server_mnemonic | reset H/M/N server_mnemonic} H=hft M=mft N=nse"
echo "Available Server mnemonics " ${!server_hostname_map[*]}
exit 1
esac
