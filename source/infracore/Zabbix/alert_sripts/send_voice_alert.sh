#!/bin/bash

## Values received by this script:
# To = $1 (Slack channel or user to send the message to, specified in the Zabbix web interface; "@username" or "#channel")
# Subject = $2 (usually either PROBLEM or RECOVERY)
# Message = $3 (whatever message the Zabbix action sends, preferably something like "Zabbix server is unreachable for 5 minutes - Zabbix server (127.0.0.1)")

# Get the Slack channel or user ($1) and Zabbix subject ($2 - hopefully either PROBLEM or RECOVERY)
to="$1"
subject="$2"

all_args=""
for var in "$2"
do
all_args=$all_args" "$var
done
ENCODED=$(echo -n $all_args | \
perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');
URL=`head -1 /spare/local/files/alert_server_url `
curl http://$URL:7980/?msg=$ENCODED > /dev/null 2>&1
