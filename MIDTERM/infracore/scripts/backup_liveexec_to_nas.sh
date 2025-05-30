#!/bin/bash
USAGE="$0 SERVER_MNEMONIC";

declare -a all_servers
declare -a all_servers=(NY11 NY12 NY13 NY14 NY15 IND11 MOS11 IND12 MOS12 TOK11 TOK12 CHI15 ASX12 BSL11 CHI14 ASX11 BSL12 BSL13 CHI16 CHI13 CFE13 BMF14 CFE12 BMF15 CFE11 BMF11 BMF12 BMF13 TOR11 TOR12 FR13 FR16 FR15 FR14 HK11 HK12);

user=$USER
if [ $# -ne 1 ] ;
then
    echo $USAGE
    echo "Existing server mnemonics: ${all_servers[*]}"
    exit;
fi

##if the server mnemonic is wrong, do not process this
if [[ ! "${all_servers[@]}" =~ "$1" ]]; then
	echo $USAGE
	echo "Existing server mnemonics: ${all_servers[*]}"
    exit;
fi

SERVER=$1;
logfile="$HOME/weekly_liveexec_sync_logs"
source_liveExec_path="/home/$user/LiveExec/"
destination_liveExec_path="/Spare/liveExec_backup/$SERVER/$user/LiveExec/"

##NAS01 10.23.74.40##
STORAGE_SERVER="10.23.74.40"

##create destination folder if doesn't exists
ssh $STORAGE_SERVER -l $user "mkdir -p /Spare/liveExec_backup/$SERVER/$user/LiveExec/" > $logfile 2>$logfile

##rsync
rsync -avz --quiet $source_liveExec_path $user@$STORAGE_SERVER:/Spare/liveExec_backup/$SERVER/$user/LiveExec/ >$logfile 2>$logfile

grep -i error $logfile | /bin/mail -s "LiveExec Weekly sync: `date +%Y%m%d`" "abhishek.anand@tworoads.co.in"
