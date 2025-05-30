#!/bin/bash

# Check if a date is provided as an argument
if [ -z "$1" ]; then
  echo "Please provide a date (YYYYMMDD) as an argument."
  exit 1
fi

running_date="$1"
YYYYMMDD=$running_date
YYYYMM=$(date -d "$running_date" +%Y%m)
MMYY=$(date -d "$running_date" +%m%y)
DDMM=$(date -d "$running_date" +%d%m)

status_file="/tmp/sync_status_${YYYYMMDD}.txt"
>$status_file

# Define machine and file maps
declare -A machines
machines["INDIBKR01"]="10.23.5.101" # TR CBOE MFT
# machines["INDIBKR02"]="10.23.5.102"
machines["local26"]="10.23.5.26"
machines["local62"]="10.23.5.62" # TR LOCAL WORKERS
machines["local67"]="10.23.5.67"
machines["local68"]="10.23.5.68"
machines["local69"]="10.23.5.69"
machines["local70"]="10.23.5.70"
machines["local71"]="10.23.5.10"
machines["local72"]="10.23.5.14"

declare -A users
users["INDIBKR01"]="dvcinfra"
users["INDIBKR02"]="dvcinfra"
users["local26"]="dvcinfra"
users["local62"]="dvcinfra"
users["local67"]="dvcinfra"
users["local68"]="dvcinfra"
users["local69"]="dvcinfra"
users["local70"]="dvcinfra"
users["local71"]="dvcinfra"
users["local72"]="dvcinfra"

# Add more machine names and IPs as needed
BASE_TI_PATH="/spare/local/tradeinfo/CBOE_Files"

declare -A files
files["Refdata"]="${BASE_TI_PATH}/RefData/cboe_fo_${YYYYMMDD}_contracts.txt"
files["Contract"]="${BASE_TI_PATH}/ContractFiles/cboe_contracts.${YYYYMMDD}"
files["Bhavcopy"]="${BASE_TI_PATH}/BhavCopy/fo/${MMYY}/${DDMM}fo_0000.md"
files["Datasource"]="${BASE_TI_PATH}/datasource_exchsymbol.txt"
# Add more files and paths as needed

# Add more machines and corresponding users as needed

# Log function to log sync status
log_sync_status() {
  local filename=$1
  local status=$2
  local machine=$3
  echo "$machine $filename $status"
}

## # Iterate over machines and files to sync
for machine in "${!machines[@]}"; do
  echo "For $machine $user"
  ip=${machines[$machine]}
  user=${users[$machine]}
  
  # Iterate over files to sync
  for file in "${!files[@]}"; do
    filepath=${files[$file]}
    
    ssh $user@$ip "mkdir -p /spare/local/tradeinfo/CBOE_Files/BhavCopy/fo/${MMYY}"
    # Perform rsync and check if it succeeded
    rsync -ravz --timeout=5 "$filepath" "$user@$ip:$filepath" > /dev/null 2>&1 && log_sync_status "$filepath" "Success" "$machine" >> $status_file || log_sync_status "$filepath" "Failed" "$machine" >> $status_file

    ssh  -o ConnectTimeout=5 $user@$ip "chown -R dvcinfra:infra /spare/local/tradeinfo/CBOE_Files"
    echo "Ownership changed"
  done
done


report_file="/tmp/status_report.html"
echo "Successfully Downloaded" > $report_file

# Special handling for 10.23.5.30 since
# it is only accessible via local 26
ssh dvcinfra@10.23.5.26 "rsync -ravz --timeout=5 --progress /spare/local/tradeinfo/CBOE_Files 10.23.5.30:/spare/local/tradeinfo/"

report_file="/tmp/report.txt"
>${report_file}

for file in "${!files[@]}"; do
  filepath=${files[$file]}
  echo "$filepath"
  /home/pengine/prod/live_scripts/check_file.sh IBKR ${filepath} | tail -n 2 | head -n 1 | awk '{print $3}' >> ${report_file}
  /home/pengine/prod/live_scripts/check_file.sh WORKER ${filepath} | tail -n 2 | head -n 1 | awk '{print $3}' >> ${report_file}
done

MAIL_FILE=/tmp/fetch.${YYYYMMDD}.mailfile
cat $(cat $report_file) > ${MAIL_FILE}

mailx -s "CBOE Fetch Status: $running_date" -r "${HOSTNAME}-${USER}<gopi.m.tatiraju@tworoads-trading.co.in>" infra_alerts@tworoads-trading.co.in gopi.m.tatiraju@tworoads-trading.co.in < "$MAIL_FILE"
