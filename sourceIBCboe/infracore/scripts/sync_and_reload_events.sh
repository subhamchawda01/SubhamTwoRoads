#!/bin/bash
#Update economic events vector on all servers
# Sync new eco events file across all servers
ssh dvctrader@10.23.74.51 'source $HOME/.profile ; /home/dvctrader/infracore/scripts/sync_merged_eco_file.sh >> /home/dvctrader/fx_reconcile_log.txt 2>&1'

all_servers=`perl $HOME/infracore/GenPerlLib/print_all_machines_vec.pl`

# Reload economic events on all servers
for server in $all_servers; do
  ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=10 dvctrader@$server << EVENTS_UPDATE_SCRIPT
  query_ids="\$(ps -efH | grep dvctrader | grep tradeinit | grep -v grep | awk '{ printf "%s ", \$12 }' )"
  echo "query_ids: \$query_ids";
  for query_id in \$query_ids ; do /home/pengine/prod/live_execs/user_msg --refreshecoevents --traderid \$query_id ; done
  for query_id in \$query_ids ; do /usr/sbin/send_user_msg --refreshecoevents --traderid \$query_id ; done
EVENTS_UPDATE_SCRIPT
done
