#! /bin/bash

declare -A normal_servers_map ;
declare -A new_servers_map ;
declare -A reverse_ip_map ;
declare -a servers_list ;

normal_servers_map[CFE11]="10.23.74.61" ;
normal_servers_map[CFE12]="10.23.74.62" ;
normal_servers_map[CFE13]="10.23.74.63" ;
normal_servers_map[FR13]="10.23.102.53" ;
normal_servers_map[FR14]="10.23.102.54" ;
normal_servers_map[CHI13]="10.23.82.53" ;
normal_servers_map[CHI14]="10.23.82.54" ;
normal_servers_map[CHI16]="10.23.82.56" ;
normal_servers_map[BSL11]="10.23.52.51" ;
normal_servers_map[BSL12]="10.23.52.52" ;
normal_servers_map[BSL13]="10.23.52.53" ;
normal_servers_map[TOR11]="10.23.182.51" ;
normal_servers_map[TOR12]="10.23.182.52" ;
normal_servers_map[BMF13]="10.220.65.33" ;
normal_servers_map[BMF14]="10.220.65.38" ;
normal_servers_map[MOS11]="172.18.244.107" ;
normal_servers_map[OSE11]="10.134.73.211" ;
normal_servers_map[OSE12]="10.134.73.212" ;
normal_servers_map[OSE13]="10.134.73.213" ;
normal_servers_map[OSE14]="10.134.73.214" ;
normal_servers_map[ASX11]="10.23.43.51" ;
normal_servers_map[ASX12]="10.23.43.52" ;
normal_servers_map[IND11]="10.23.27.3" ;
normal_servers_map[IND12]="10.23.27.2" ;
normal_servers_map[SGX11]="10.23.26.51" ;
normal_servers_map[SGX12]="10.23.26.52" ;
normal_servers_map[CHI15]="10.23.82.55" ;

new_servers_map[FR15]="10.23.102.55" ;
new_servers_map[FR16]="10.23.102.56" ;
new_servers_map[BMF11]="10.220.65.35" ;
new_servers_map[BMF12]="10.220.65.34" ;
new_servers_map[MOS12]="10.23.241.2";
new_servers_map[BMF15]="10.220.65.36" ;
new_servers_map[IND13]="10.23.27.4" ;

remote_user="dvcinfra@";

process_service() {
  operation=$1
  shift
  servers_list=( "$@" )
  #if no server listed do the operation for ALL servers 
  if [ -z $servers_list ] ; then
    servers_list="ALL";
  fi
  if [ "$servers_list" == "ALL" ] ; then 
    servers_list=`echo ${!normal_servers_map[*]} ${!new_servers_map[*]}` ;
    echo $servers_list ;
  fi

  for server in `echo ${servers_list[*]}` ;
  do
    printf "$server : "
    if [ ${normal_servers_map[$server]+_} ] ; then
        machine=${normal_servers_map[$server]} ;
        ssh -T $remote_user$machine << EOF 
          service dvc_pushdelta $operation
#grep 'diff' /home/pengine/prod/live_scripts/get_delta_trades.sh
EOF

    elif [ ${new_servers_map[$server]+_} ] ; then
        machine=${new_servers_map[$server]}
        ssh -T $remote_user$machine << EOF
          sudo systemctl $operation dvc_pushdelta | grep active
#          grep 'diff' /home/pengine/prod/live_scripts/get_delta_trades.sh
          if [ "$?" -eq "0" -a $operation != "status" ] ; then
            echo "DONE"
          fi
EOF
    else
        echo "Server $server not found"
        exit 1
    fi
  done
}


# See how we were called.
case "$1" in
START|start)
     shift
     process_service start $@
     ;;
STOP|stop)
     shift
     process_service stop $@
     ;;
RESTART|restart)
     shift
     process_service stop $@
     process_service start $@
     ;;
STATUS|status)
     shift
     process_service status $@
     ;;
*)
    echo "Usage: $0 {start|stop|restart|status} {server_list}"
    echo "Prod Servers: " ${!normal_servers_map[*]} ${!new_servers_map[*]} ALL
    exit 1
esac

